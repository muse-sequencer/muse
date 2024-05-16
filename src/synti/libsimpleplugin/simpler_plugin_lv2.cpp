//=========================================================
//  MusE
//  Linux Music Editor
//  simpler_plugin_lv2.cpp
//  (C) Copyright 2018 Tim E. Real (terminator356 on users.sourceforge.net)
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

// #include <QtCore>
// #include <QtWidgets>
// #include <stdlib.h>
// #include <stdio.h>
// #include <unistd.h>
// #include <dlfcn.h>

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
#include <QDirIterator>
#include <QInputDialog>

#include <QDir>
#include <QFileInfo>
#include <QUrl>
//#include <QX11EmbedWidget>
#include <QCoreApplication>
#include <QtGui/QWindow>
#include <QVBoxLayout>

#include "simpler_plugin_lv2.h"
// #include "synth.h"
#include "audio.h"
// #include "jackaudio.h"
#include "midi_consts.h"
#include "midiport.h"
#include "stringparam.h"
#include "plugin.h"
#include "controlfifo.h"
#include "xml.h"
#include "song.h"
#include "ctrl.h"
// #include "minstrument.h"

#include "app.h"
#include "globals.h"
#include "globaldefs.h"
#include "gconfig.h"
#include "widgets/popupmenu.h"
#include "widgets/menutitleitem.h"
#include "icons.h"
#include <ladspa.h>

#include <cmath>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sord/sord.h>


#define SS_LOG_MAX   0
#define SS_LOG_MIN -10
#define SS_LOG_OFFSET SS_LOG_MIN


#define SP_TRACE_FUNC   0
#define SP_DEBUG_LADSPA 0   

#define SP_TRACE_IN if (SP_TRACE_FUNC) fprintf (stderr, "->%s:%d\n", __PRETTY_FUNCTION__, __LINE__);
#define SP_TRACE_OUT if (SP_TRACE_FUNC) fprintf (stderr, "<-%s:%d\n", __PRETTY_FUNCTION__, __LINE__);
#define SP_ERROR(string) fprintf(stderr, "SimplePlugin error: %s\n", string)
#define SP_DBG_LV2(string1) if (SP_DEBUG_LV2) fprintf(stderr, "%s:%d:%s: %s\n", __FILE__ , __LINE__ , __PRETTY_FUNCTION__, string1);
#define SP_DBG_LV2_2(string1, string2) if (SP_DEBUG_LV2) fprintf(stderr, "%s:%d:%s: %s: %s\n", __FILE__ , __LINE__ , __PRETTY_FUNCTION__, string1, string2);

// Turn on debugging messages.
//#define PLUGIN_DEBUGIN

// Turn on constant stream of debugging messages.
//#define PLUGIN_DEBUGIN_PROCESS




// Uncomment to print audio process info.
//#define LV2_DEBUG_PROCESS

// Uncomment to print general info.
// (There is also the CMake option LV2_DEBUG for more output.)
//#define LV2_DEBUG

// #ifdef HAVE_GTK2
// #include "lv2Gtk2Support/lv2Gtk2Support.h"
// #endif

// Define to use GtkPlug instead of GtkWindow for a Gtk plugin gui container.
// This works better than GtkWindow for some plugins.
// For example with GtkWindow, AMSynth fails to embed into the container window
//  resulting in two separate windows.
// #define LV2_GUI_USE_GTKPLUG ;


namespace MusESimplePlugin {


#define NS_EXT "http://lv2plug.in/ns/ext/"
#define NS_LV2CORE "http://lv2plug.in/ns/lv2core"

#define LV2_INSTRUMENT_CLASS NS_LV2CORE "#InstrumentPlugin"
#define LV2_F_BOUNDED_BLOCK_LENGTH LV2_BUF_SIZE__boundedBlockLength
#define LV2_F_FIXED_BLOCK_LENGTH LV2_BUF_SIZE__fixedBlockLength
#define LV2_F_POWER_OF_2_BLOCK_LENGTH LV2_BUF_SIZE__powerOf2BlockLength
// BUG FIXME: 'coarseBlockLength' is NOT in the lv2 buf-size.h header file!
// #define LV2_F_COARSE_BLOCK_LENGTH LV2_BUF_SIZE__coarseBlockLength
#define LV2_F_COARSE_BLOCK_LENGTH LV2_BUF_SIZE_PREFIX "coarseBlockLength"
#define LV2_P_SEQ_SIZE LV2_BUF_SIZE__sequenceSize
#define LV2_P_MAX_BLKLEN LV2_BUF_SIZE__maxBlockLength
#define LV2_P_MIN_BLKLEN LV2_BUF_SIZE__minBlockLength
#define LV2_P_NOM_BLKLEN LV2_BUF_SIZE__nominalBlockLength
#define LV2_P_SAMPLE_RATE LV2_PARAMETERS__sampleRate
#define LV2_F_OPTIONS LV2_OPTIONS__options
#define LV2_F_URID_MAP LV2_URID__map
#define LV2_F_URID_UNMAP LV2_URID__unmap
#ifdef LV2_URI_MAP_SUPPORT
#define LV2_F_URI_MAP LV2_URI_MAP_URI
#endif
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
#define LV2_F_STATE_CHANGED LV2_STATE_PREFIX "StateChanged"


static LilvWorld *lilvWorld = 0;
static int uniqueID = 1;

//uri cache structure.
typedef struct
{
   LilvNode *atom_AtomPort;
#ifdef LV2_EVENT_BUFFER_SUPPORT
   LilvNode *ev_EventPort;
#endif
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
   LilvNode *lv2_actionSavePreset;
   LilvNode *lv2_actionUpdatePresets;
   LilvNode *end;  ///< NULL terminator for easy freeing of entire structure
} CacheNodes;

LV2_URID Synth_Urid_Map(LV2_URID_Unmap_Handle _host_data, const char *uri)
{
//    LV2Synth *_synth = reinterpret_cast<LV2Synth *>(_host_data);
   Lv2Plugin *_synth = reinterpret_cast<Lv2Plugin *>(_host_data);

   if(_synth == nullptr)   //broken plugin
   {
      return 0;
   }

   return _synth->mapUrid(uri);
}

const char *Synth_Urid_Unmap(LV2_URID_Unmap_Handle _host_data, LV2_URID id)
{
//    LV2Synth *_synth = reinterpret_cast<LV2Synth *>(_host_data);
   Lv2Plugin *_synth = reinterpret_cast<Lv2Plugin *>(_host_data);

   if(_synth == nullptr)   //broken plugin
   {
      return NULL;
   }

   return _synth->unmapUrid(id);
}

#ifdef LV2_URI_MAP_SUPPORT
LV2_URID Synth_Uri_Map(LV2_URI_Map_Callback_Data _host_data, const char *, const char *uri)
{
//    LV2Synth *_synth = reinterpret_cast<LV2Synth *>(_host_data);
   Lv2Plugin *_synth = reinterpret_cast<Lv2Plugin *>(_host_data);

   if(_synth == nullptr)   //broken plugin
   {
      return 0;
   }

   return _synth->mapUrid(uri);
}
#endif


static CacheNodes lv2CacheNodes;

LV2_Feature lv2Features [] =
{
   {LV2_F_URID_MAP, nullptr},
   {LV2_F_URID_UNMAP, nullptr},
#ifdef LV2_URI_MAP_SUPPORT
   {LV2_F_URI_MAP, nullptr},
#endif
   {LV2_F_BOUNDED_BLOCK_LENGTH, nullptr},
   {LV2_F_FIXED_BLOCK_LENGTH, nullptr},
   {LV2_F_POWER_OF_2_BLOCK_LENGTH, nullptr},
   {LV2_F_COARSE_BLOCK_LENGTH, nullptr},
   {LV2_F_UI_PARENT, nullptr},
   {LV2_F_INSTANCE_ACCESS, nullptr},
   {LV2_F_UI_EXTERNAL_HOST, nullptr},
   {LV2_UI_EXTERNAL_DEPRECATED, nullptr},
   {LV2_F_WORKER_SCHEDULE, nullptr},
   {LV2_F_UI_IDLE, nullptr},
   {LV2_F_OPTIONS, nullptr},
   {LV2_UI__resize, nullptr},
   {LV2_PROGRAMS__Host, nullptr},
   {LV2_LOG__log, nullptr},
   {LV2_STATE__makePath, nullptr},
   {LV2_STATE__mapPath, nullptr},
   {LV2_F_STATE_CHANGED, nullptr},
   {LV2_F_DATA_ACCESS, nullptr} //must be the last always!
};

std::set<std::string> supportedFeatures;

// std::vector<LV2Synth *> synthsToFree;
std::vector<Lv2Plugin *> synthsToFree;

#define SIZEOF_ARRAY(x) sizeof(x)/sizeof(x[0])

// static
// Pointer to this required for LV2_P_MIN_BLKLEN option when
//  composing LV2_Options_Option array in Lv2Plugin::Lv2Plugin().
const unsigned Lv2Plugin::minBlockSize = 0U;

void initLV2()
{
// #ifdef HAVE_GTK2
//    //----------------- 
//    // Initialize Gtk
//    //----------------- 
//    MusEGui::lv2Gtk2Helper_init();
// #endif
      
//    std::set<std::string> supportedFeatures;
   uint32_t i = 0;

   if(MusEGlobal::debugMsg)
     std::cerr << "LV2: MusE supports these features:" << std::endl;
     
   for(i = 0; i < SIZEOF_ARRAY(lv2Features); i++)
   {
      supportedFeatures.insert(lv2Features [i].URI);
      if(MusEGlobal::debugMsg)
        std::cerr << "\t" << lv2Features [i].URI << std::endl;
   }

   lilvWorld = lilv_world_new();

   lv2CacheNodes.atom_AtomPort          = lilv_new_uri(lilvWorld, LV2_ATOM__AtomPort);
#ifdef LV2_EVENT_BUFFER_SUPPORT
   lv2CacheNodes.ev_EventPort           = lilv_new_uri(lilvWorld, LV2_EVENT__EventPort);
#endif
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
   lv2CacheNodes.lv2_rdfsLabel          = lilv_new_uri(lilvWorld, "http://www.w3.org/2000/01/rdf-schema#label");
   lv2CacheNodes.lv2_actionSavePreset   = lilv_new_uri(lilvWorld, ORGANIZATION_URL "lv2host#lv2_actionSavePreset");
   lv2CacheNodes.lv2_actionUpdatePresets= lilv_new_uri(lilvWorld, ORGANIZATION_URL "lv2host#lv2_actionUpdatePresets");
   lv2CacheNodes.end                    = nullptr;

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
         pit = lilv_plugins_next(plugins, pit);
         continue;
      }

      LilvNode *nameNode = lilv_plugin_get_name(plugin);
      //const LilvNode *uriNode = lilv_plugin_get_uri(plugin);

      if(lilv_node_is_string(nameNode))
      {
         bool shouldLoad = true;
         const char *pluginName = lilv_node_as_string(nameNode);
         //const char *pluginUri = lilv_node_as_string(uriNode);
         if(MusEGlobal::debugMsg)
           std::cerr << "Found LV2 plugin: " << pluginName << std::endl;
         // lilv_uri_to_path is deprecated. Use lilv_file_uri_parse instead. Return value must be freed with lilv_free.
         const char *lfp = lilv_file_uri_parse(lilv_node_as_string(lilv_plugin_get_library_uri(plugin)), nullptr);
         if(MusEGlobal::debugMsg)
           std::cerr << "Library path: " << lfp << std::endl;

         if(MusEGlobal::debugMsg)
         {
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
         }

#ifdef DEBUG_LV2
         std::cerr <<  "\tRequired features (by uri):" << std::endl;
#endif
         LilvNodes *fts = lilv_plugin_get_required_features(plugin);
         LilvIter *nit = lilv_nodes_begin(fts);

         Plugin::PluginFeaturesType reqfeat = Plugin::NoFeatures;
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

            if(isSupported)
            {
              if(strcmp(uri, LV2_F_FIXED_BLOCK_LENGTH) == 0)
                reqfeat |= Plugin::FixedBlockSize;
              else if(strcmp(uri, LV2_F_POWER_OF_2_BLOCK_LENGTH) == 0)
                reqfeat |= Plugin::PowerOf2BlockSize;
              else if(strcmp(uri, LV2_F_COARSE_BLOCK_LENGTH) == 0)
                reqfeat |= Plugin::CoarseBlockLength;
            }
            else
            {
               shouldLoad = false;
               std::cerr << "\t LV2: " << pluginName << ": Required feature: " << uri << ": not supported!" << std::endl;
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
//             // Make sure it doesn't already exist.
//             std::vector<Synth *>::iterator is;
// 
//             for(is = MusEGlobal::synthis.begin(); is != MusEGlobal::synthis.end(); ++is)
//             {
//                Synth *s = *is;
// 
//                if(s->name() == name && s->baseName() == fi.completeBaseName())
//                {
//                   break;
//                }
//             }
// 
//             if(is == MusEGlobal::synthis.end())
              
              
            // Make sure it doesn't already exist.
            if(MusESimplePlugin::plugins.find(fi.completeBaseName(), name) == 0)
            {
               LilvNode *nAuthor = lilv_plugin_get_author_name(plugin);
               QString author;

               if(nAuthor != nullptr)
               {
                  author = lilv_node_as_string(nAuthor);
                  lilv_node_free(nAuthor);
               }

               //LV2Synth *s = new LV2Synth(fi, name, name, author, plugin, reqfeat);
               Lv2Plugin *s = new Lv2Plugin(&fi, name, name, author, plugin, reqfeat);
               
               if(s->isConstructed())
               {

//                   if((s->isSynth() && s->outPorts() > 0)
//                           || (s->inPorts() > 0 && s->outPorts() > 0))
//                       //insert plugins with audio ins and outs to synths list too
//                   {
//                      MusEGlobal::synthis.push_back(s);
//                   }
//                   else
//                   {
//                      synthsToFree.push_back(s);
//                   }
// 
//                   if(s->inPorts() > 0 && s->outPorts() > 0)   // insert to plugin list
//                   {
//                      MusEGlobal::plugins.push_back(new LV2PluginWrapper(s, reqfeat));
// 
//                   }
                 
                  MusESimplePlugin::plugins.push_back(s);
               }
               else
               {
                  delete s;
               }

            }
         }
         lilv_free((void*)lfp); // Must free.
      }

      if(nameNode != nullptr)
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
   synthsToFree.clear();

   for(LilvNode **n = (LilvNode **)&lv2CacheNodes; *n; ++n)
   {
      lilv_node_free(*n);
   }

// #ifdef HAVE_GTK2
//    MusEGui::lv2Gtk2Helper_deinit();
// #endif

   lilv_world_free(lilvWorld);
   lilvWorld = nullptr;

}

// void LV2Synth::lv2ui_ExtUi_Closed(LV2UI_Controller contr)
// {
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)contr;
//    assert(state != nullptr); //this shouldn't happen
//    assert(state->widget != nullptr); // this too
//    assert(state->pluginWindow != nullptr);
// 
//    state->pluginWindow->setClosing(true);
// 
// 
//    //state->uiTimer->stopNextTime(false);
// }
// 
// void LV2Synth::lv2ui_SendChangedControls(LV2PluginWrapper_State *state)
// {
//    if(state != nullptr && state->uiDesc != nullptr && state->uiDesc->port_event != nullptr && state->uiInst != nullptr)
//    {      
//       size_t numControls = 0;
//       MusECore::Port *controls = nullptr;
//       size_t numControlsOut = 0;
//       MusECore::Port *controlsOut = nullptr;
//       LV2Synth *synth = state->synth;
// 
//       if(state->plugInst != nullptr)
//       {
//          numControls = state->plugInst->controlPorts;
//          controls = state->plugInst->controls;
//          numControlsOut = state->plugInst->controlOutPorts;
//          controlsOut = state->plugInst->controlsOut;
// 
//       }
//       else if(state->sif != nullptr)
//       {
//          numControls = state->sif->_inportsControl;
//          controls = state->sif->_controls;
//          numControlsOut = state->sif->_outportsControl;
//          controlsOut = state->sif->_controlsOut;
//       }
// 
//       if(numControls > 0)
//       {
//          assert(controls != nullptr);
//       }
// 
//       if(numControlsOut > 0)
//       {
//          assert(controlsOut != nullptr);
//       }
//       for(uint32_t i = 0; i < numControls; ++i)
//       {
//          if(state->controlTimers [i] > 0)
//          {
//             --state->controlTimers [i];
//             continue;
//          }
//          if(state->controlsMask [i])
//          {
//             state->controlsMask [i] = false;
// 
//             // Force send if re-opening.
//             if(state->uiIsOpening || state->lastControls [i] != controls [i].val)
//             {
//                state->lastControls [i] = controls [i].val;
//                state->uiDesc->port_event(state->uiInst,
//                                         controls [i].idx,
//                                         sizeof(float), 0,
//                                         &controls [i].val);
//             }
//          }
//       }
// 
//       for(uint32_t i = 0; i < numControlsOut; ++i)
//       {
//          // Force send if re-opening.
//          if(state->uiIsOpening || state->lastControlsOut [i] != controlsOut [i].val)
//          {
//             state->lastControlsOut [i] = controlsOut [i].val;
//             state->uiDesc->port_event(state->uiInst,
//                                      controlsOut [i].idx,
//                                      sizeof(float), 0,
//                                      &controlsOut [i].val);
//          }
// 
//       }
// 
//       //process gui atom events (control events are already set by getData or apply.
//       size_t fifoItemSize = state->plugControlEvt.getItemSize();
//       size_t dataSize = 0;
//       uint32_t port_index = 0;
//       char evtBuffer [fifoItemSize];
//       while(state->plugControlEvt.get(&port_index, &dataSize, evtBuffer))
//       {
//          state->uiDesc->port_event(state->uiInst, port_index, dataSize, synth->_uAtom_EventTransfer, evtBuffer);
//       }
//    }
// }
// 
// 
// 
// void LV2Synth::lv2ui_PortWrite(LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t protocol, void const *buffer)
// {
//    LV2Synth::lv2state_PortWrite(controller, port_index, buffer_size, protocol, buffer, true);
// }
// 
// void LV2Synth::lv2ui_Touch(LV2UI_Controller /*controller*/, uint32_t /*port_index*/, bool grabbed __attribute__ ((unused)))
// {
// #ifdef DEBUG_LV2
//    std::cerr << "LV2Synth::lv2ui_UiTouch: port: %u " << (grabbed ? "grabbed" : "released") << std::endl;
// #endif
// 
// }
// 
// 

void Lv2Plugin::lv2_FillFeatures(LV2_Feature *features, LV2_Feature **ppfeatures, LV2_Options_Option* options)
{
   uint32_t i;
   for(i = 0; i < SIZEOF_ARRAY(lv2Features); i++)
   {
      features [i] = _features [i];

      if(features [i].URI == nullptr)
      {
         break;
      }

      if(i == _fOptions)
      {
         features [i].data = options;
      }
      else if(i == _fInstanceAccess)
      {
         features [i].data = nullptr;
      }
      else if(i == _fExtUiHost)
      {
         features [i].data = &state->extHost;
      }
      else if(i == _fExtUiHostD)
      {
         features [i].data = &state->extHost;
      }
      else if(i == _fDataAccess)
      {
         features [i].data = &state->extData;
      }
      else if(i == _fWrkSchedule)
      {
         features [i].data = &state->wrkSched;
      }
      else if(i == _fUiResize)
      {
         features [i].data = &state->uiResize;
      }
      else if(i == _fPrgHost)
      {
         features [i].data = &state->prgHost;
      }
      else if(i == _fMakePath)
      {
         features [i].data = &state->makePath;
      }
      else if(i == _fMapPath)
      {
         features [i].data = &state->mapPath;
      }

//       _ppifeatures [i] = &_ifeatures [i];
      ppfeatures [i] = &features [i];
   }

//    _ppifeatures [i] = nullptr;
   ppfeatures [i] = nullptr;
}

void Lv2Plugin::lv2state_FillFeatures(LV2PluginWrapper_State *state)
{
   uint32_t i;
//    LV2Synth *synth = state->synth;
   Lv2Plugin *synth = state->synth;
   LV2_Feature *_ifeatures = state->_ifeatures;
   LV2_Feature **_ppifeatures = state->_ppifeatures;

   //state->uiTimer = new LV2PluginWrapper_Timer(state);

   state->wrkSched.handle = (LV2_Worker_Schedule_Handle)state;
//    state->wrkSched.schedule_work = LV2Synth::lv2wrk_scheduleWork;
   state->wrkSched.schedule_work = lv2wrk_scheduleWork;
   state->wrkIface = nullptr;
   state->wrkThread = new LV2PluginWrapper_Worker(state);

   state->extHost.plugin_human_id = state->human_id = nullptr;
   state->extHost.ui_closed = LV2Synth::lv2ui_ExtUi_Closed;

   state->extData.data_access = nullptr;

   for(i = 0; i < SIZEOF_ARRAY(lv2Features); i++)
   {
      _ifeatures [i] = synth->_features [i];

      if(_ifeatures [i].URI == nullptr)
      {
         break;
      }

      if(i == synth->_fInstanceAccess)
      {
         _ifeatures [i].data = nullptr;
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

   _ppifeatures [i] = nullptr;

   state->curBpm = 0.0;//double(60000000.0/MusEGlobal::tempomap.tempo(MusEGlobal::song->cpos()));
   state->curIsPlaying = MusEGlobal::audio->isPlaying();
   state->curFrame = MusEGlobal::audioDevice->getCurFrame();
   lv2_atom_forge_init(&state->atomForge, &synth->_lv2_urid_map);

   LV2Synth::lv2state_InitMidiPorts(state);
}

// void LV2Synth::lv2state_PostInstantiate(LV2PluginWrapper_State *state)
// {
//    LV2Synth *synth = state->synth;
//    const LV2_Descriptor *descr = lilv_instance_get_descriptor(state->handle);
// 
//    state->_ifeatures [synth->_fInstanceAccess].data = lilv_instance_get_handle(state->handle);
// 
//    if(descr->extension_data != nullptr)
//    {
//       state->extData.data_access = descr->extension_data;
//    }
//    else
//    {
//       state->_ppifeatures [synth->_fDataAccess] = nullptr;
//    }
// 
//    state->controlsNameMap.clear();
// 
//    size_t nCpIn = synth->_controlInPorts.size();
//    size_t nCpOut = synth->_controlOutPorts.size();
// 
//    if(nCpIn > 0)
//    {
//       state->lastControls = new float [nCpIn];
//       state->controlsMask = new bool [nCpIn];
//       state->controlTimers = new int [nCpIn];
//       for(uint32_t i = 0; i < nCpIn; i++)
//       {
//          state->lastControls [i] = synth->_pluginControlsDefault [synth->_controlInPorts [i].index];
//          state->controlsMask [i] = false;
//          state->controlTimers [i] = 0;
//          state->controlsNameMap.insert(std::pair<QString, size_t>(QString(synth->_controlInPorts [i].cName).toLower(), i));
//          state->controlsSymMap.insert(std::pair<QString, size_t>(QString(synth->_controlInPorts [i].cSym).toLower(), i));
//       }
//    }
// 
//    if(nCpOut > 0)
//    {
//       state->lastControlsOut = new float [nCpOut];
//       for(uint32_t i = 0; i < nCpOut; i++)
//       {
//          state->lastControlsOut [i] = synth->_pluginControlsDefault [synth->_controlOutPorts [i].index];
//       }
//    }
// 
//    //fill pointers for CV port types;
// 
//    uint32_t numAllPorts = lilv_plugin_get_num_ports(synth->_handle);
// 
//    state->pluginCVPorts = new float *[numAllPorts];
//    int rv = posix_memalign((void **)&state->pluginCVPorts, 16, sizeof(float *) * numAllPorts);
//    if(rv != 0)
//    {
//       fprintf(stderr, "ERROR: LV2Synth::lv2state_PostInstantiate: posix_memalign returned error:%d. Aborting!\n", rv);
//       abort();
//    }
// 
//    memset(state->pluginCVPorts, 0, sizeof(float *) * numAllPorts);
// 
//    for(size_t i = 0; i < synth->_controlInPorts.size(); ++i)
//    {
//       if(synth->_controlInPorts [i].isCVPort)
//       {
//          size_t idx = synth->_controlInPorts [i].index;
//          rv = posix_memalign((void **)&state->pluginCVPorts [idx], 16, sizeof(float) * MusEGlobal::segmentSize);
// 
//          if(rv != 0)
//          {
//             fprintf(stderr, "ERROR: LV2Synth::lv2state_PostInstantiate: posix_memalign returned error:%d. Aborting!\n", rv);
//             abort();
//          }
//          for(size_t k = 0; k < MusEGlobal::segmentSize; ++k)
//          {
//             state->pluginCVPorts [idx] [k] = synth->_controlInPorts [i].defVal;
//          }
//          lilv_instance_connect_port(state->handle, idx, state->pluginCVPorts [idx]);
//       }
//    }
// 
//    for(size_t i = 0; i < synth->_controlOutPorts.size(); ++i)
//    {
//       if(synth->_controlOutPorts [i].isCVPort)
//       {
//          size_t idx = synth->_controlOutPorts [i].index;
//          rv = posix_memalign((void **)&state->pluginCVPorts [idx], 16, sizeof(float) * MusEGlobal::segmentSize);
// 
//          if(rv != 0)
//          {
//             fprintf(stderr, "ERROR: LV2Synth::lv2state_PostInstantiate: posix_memalign returned error:%d. Aborting!\n", rv);
//             abort();
//          }
//          for(size_t k = 0; k < MusEGlobal::segmentSize; ++k)
//          {
//             state->pluginCVPorts [idx] [k] = synth->_controlOutPorts [i].defVal;
//          }
//          lilv_instance_connect_port(state->handle, idx, state->pluginCVPorts [idx]);
//       }
//    }
// 
//    for(size_t i = 0; i < state->midiInPorts.size(); i++)
//    {
//       lilv_instance_connect_port(state->handle, state->midiInPorts [i].index, (void *)state->midiInPorts [i].buffer->getRawBuffer());
//    }
// 
//    for(size_t i = 0; i < state->midiOutPorts.size(); i++)
//    {
//      lilv_instance_connect_port(state->handle, state->midiOutPorts [i].index, (void *)state->midiOutPorts [i].buffer->getRawBuffer());
//    }
// 
//    //query for state interface
//    state->iState = (LV2_State_Interface *)lilv_instance_get_extension_data(state->handle, LV2_STATE__interface);
//    //query for LV2Worker interface
//    state->wrkIface = (LV2_Worker_Interface *)lilv_instance_get_extension_data(state->handle, LV2_F_WORKER_INTERFACE);
//    //query for programs interface   
//    state->prgIface = (LV2_Programs_Interface *)lilv_instance_get_extension_data(state->handle, LV2_PROGRAMSNEW__Interface);
//    if(state->prgIface != nullptr)
//    {
//       state->newPrgIface = true;
//    }
//    else
//    {
//       state->newPrgIface = false;
//       state->prgIface = (LV2_Programs_Interface *)lilv_instance_get_extension_data(state->handle, LV2_PROGRAMS__Interface);
//    }
// 
//    LV2Synth::lv2prg_updatePrograms(state);
// 
//    state->wrkThread->start(QThread::LowPriority);
// 
// }



// TODO: UI
// void Lv2Plugin::lv2ui_FreeDescriptors(LV2PluginWrapper_State *state)
// {
//    if(state->uiDesc != nullptr && state->uiInst != nullptr)
//       state->uiDesc->cleanup(state->uiInst);
// 
//    state->uiInst = *(void **)(&state->uiDesc) = nullptr;
// 
// // #ifdef HAVE_GTK2
// //    if(state->gtk2Plug != nullptr)
// //      MusEGui::lv2Gtk2Helper_gtk_widget_destroy(state->gtk2Plug);
// // #endif      
// 
//    state->gtk2Plug = nullptr;
// 
//    if(state->uiDlHandle != nullptr)
//    {
//       dlclose(state->uiDlHandle);
//       state->uiDlHandle = nullptr;
//    }
// 
// }

void Lv2Plugin::lv2state_FreeState(LV2PluginWrapper_State *state)
{
   assert(state != nullptr);

// TODO: wrk
//    state->wrkThread->setClosing();
//    state->wrkThread->wait();
   delete state->wrkThread;

   if(state->human_id != nullptr)
      free(state->human_id);
   if(state->lastControls)
   {
      delete [] state->lastControls;
      state->lastControls = nullptr;
   }
   if(state->controlsMask)
   {
      delete [] state->controlsMask;
      state->controlsMask = nullptr;
   }

   if(state->controlTimers)
   {
      delete [] state->controlTimers;
      state->controlTimers = nullptr;

   }

   if(state->lastControlsOut)
   {
      delete [] state->lastControlsOut;
      state->lastControlsOut = nullptr;
   }

// TODO: UI
   //lv2ui_FreeDescriptors(state);

   if(state->handle != nullptr)
   {
      lilv_instance_free(state->handle);
      state->handle = nullptr;
   }

   delete state;
}

// void LV2Synth::lv2audio_SendTransport(LV2PluginWrapper_State *state, LV2EvBuf *buffer, unsigned long nsamp)
// {
//    //send transport events if any
//    LV2Synth *synth = state->synth;
//    unsigned int cur_frame = MusEGlobal::audio->pos().frame();
//    Pos p(MusEGlobal::extSyncFlag ? MusEGlobal::audio->tickPos() : cur_frame, MusEGlobal::extSyncFlag ? true : false);
//    float curBpm = (float)MusEGlobal::tempomap.globalTempo() * 600000.0f / (float)MusEGlobal::tempomap.tempo(p.tick());
//    bool curIsPlaying = MusEGlobal::audio->isPlaying();
//    unsigned int curFrame = MusEGlobal::audioDevice->getCurFrame();
//    //   if(state->curFrame != curFrame
//    //      || state->curIsPlaying != curIsPlaying
//    //      || state->curBpm != curBpm)
//    //   {
// 
//    //send transport/tempo changes always
//    //as some plugins revert to default settings when not playing
//    state->curFrame = curFrame;
//    state->curIsPlaying = curIsPlaying;
//    state->curBpm = curBpm;
//    uint8_t   pos_buf[1024];
//    memset(pos_buf, 0, sizeof(pos_buf));
//    LV2_Atom* lv2_pos = (LV2_Atom*)pos_buf;
//    /* Build an LV2 position object to report change to plugin */
//    LV2_Atom_Forge* atomForge = &state->atomForge;
//    lv2_atom_forge_set_buffer(atomForge, pos_buf, sizeof(pos_buf));
//    LV2_Atom_Forge_Frame frame;
//    lv2_atom_forge_object(atomForge, &frame, 1, synth->_uTime_Position);
//    lv2_atom_forge_key(atomForge, synth->_uTime_frame);
//    lv2_atom_forge_long(atomForge, curFrame);
//    lv2_atom_forge_key(atomForge, synth->_uTime_speed);
//    lv2_atom_forge_float(atomForge, curIsPlaying ? 1.0 : 0.0);
//    lv2_atom_forge_key(atomForge, synth->_uTime_beatsPerMinute);
//    lv2_atom_forge_float(atomForge, curBpm);
// #ifdef LV2_EVENT_BUFFER_SUPPORT
//    buffer->write(nsamp, 0, lv2_pos->type, lv2_pos->size, (const uint8_t *)LV2_ATOM_BODY(lv2_pos));
// #else
//    buffer->write(nsamp, lv2_pos->type, lv2_pos->size, (const uint8_t *)LV2_ATOM_BODY(lv2_pos));
// #endif
// }
// 
// void LV2Synth::lv2state_InitMidiPorts(LV2PluginWrapper_State *state)
// {
//    LV2Synth *synth = state->synth;
//    state->midiInPorts = synth->_midiInPorts;
//    state->midiOutPorts = synth->_midiOutPorts;
//    state->inPortsMidi= state->midiInPorts.size();
//    state->outPortsMidi = state->midiOutPorts.size();
//    //connect midi and control ports
//    for(size_t i = 0; i < state->midiInPorts.size(); i++)
//    {
//       LV2EvBuf *newEvBuffer = new LV2EvBuf(
//            true,
// #ifdef LV2_EVENT_BUFFER_SUPPORT
//            state->midiInPorts [i].old_api, 
// #endif
//            synth->_uAtom_Sequence, 
//            synth->_uAtom_Chunk);
//       if(!newEvBuffer)
//       {
//          abort();
//       }
//       state->midiInPorts [i].buffer = newEvBuffer;
//       state->idx2EvtPorts.insert(std::make_pair(state->midiInPorts [i].index, newEvBuffer));
//    }
// 
//    for(size_t i = 0; i < state->midiOutPorts.size(); i++)
//    {      
//       LV2EvBuf *newEvBuffer = new LV2EvBuf(
//            false, 
// #ifdef LV2_EVENT_BUFFER_SUPPORT
//            state->midiOutPorts [i].old_api, 
// #endif
//            synth->_uAtom_Sequence, 
//            synth->_uAtom_Chunk);
//       if(!newEvBuffer)
//       {
//          abort();
//       }
//       state->midiOutPorts [i].buffer = newEvBuffer;
//       state->idx2EvtPorts.insert(std::make_pair(state->midiOutPorts [i].index, newEvBuffer));
//    }
// 
// }
// 
// void LV2Synth::lv2audio_preProcessMidiPorts(LV2PluginWrapper_State *state, unsigned long nsamp)
// {
//    for(size_t j = 0; j < state->inPortsMidi; j++)
//    {
//       state->midiInPorts [j].buffer->resetBuffer();
//    }
// 
//    for(size_t j = 0; j < state->outPortsMidi; j++)
//    {
//       state->midiOutPorts [j].buffer->resetBuffer();
//    }
// 
//    if(state->inPortsMidi > 0)
//    {
//       LV2EvBuf *rawMidiBuffer = state->midiInPorts [0].buffer;
// 
//       if(state->midiInPorts [0].supportsTimePos)
//       {
//          //send transport events if any
//          LV2Synth::lv2audio_SendTransport(state, rawMidiBuffer, nsamp);
//       }
//    }
// 
//    //process gui atom events (control events are already set by getData or apply call).
//    size_t fifoItemSize = state->uiControlEvt.getItemSize();
//    size_t dataSize = 0;
//    uint32_t port_index = 0;
//    char evtBuffer [fifoItemSize];
//    while(state->uiControlEvt.get(&port_index, &dataSize, evtBuffer))
//    {
//       std::map<uint32_t, LV2EvBuf *>::iterator it = state->idx2EvtPorts.find(port_index);
//       if(it != state->idx2EvtPorts.end())
//       {
//          LV2EvBuf *buffer = it->second;
//          const LV2_Atom* const atom = (const LV2_Atom*)evtBuffer;
// #ifdef LV2_EVENT_BUFFER_SUPPORT
//          buffer->write(nsamp, 0, atom->type, atom->size,  static_cast<const uint8_t *>(LV2_ATOM_BODY_CONST(atom)));
// #else
//          buffer->write(nsamp, atom->type, atom->size,  static_cast<const uint8_t *>(LV2_ATOM_BODY_CONST(atom)));
// #endif
//       }
// 
//    }
// }
// 
// void LV2Synth::lv2audio_postProcessMidiPorts(LV2PluginWrapper_State *state, unsigned long)
// {
//    //send Atom events to gui.
//    //Synchronize send rate with gui update rate
//    
// 
//    size_t fifoItemSize = state->plugControlEvt.getItemSize();
// 
//    size_t outp = state->midiOutPorts.size();
// 
//    for(size_t j = 0; j < outp; j++)
//    {
// #ifdef LV2_EVENT_BUFFER_SUPPORT
//       if(!state->midiOutPorts [j].old_api)
// #endif
//       {
//          do
//          {
//             uint32_t frames, type, size;
//             uint8_t *data = nullptr;
// #ifdef LV2_EVENT_BUFFER_SUPPORT
//             uint32_t subframes;
//             if(!state->midiOutPorts [j].buffer->read(&frames, &subframes, &type, &size, &data))
// #else
//             if(!state->midiOutPorts [j].buffer->read(&frames, &type, &size, &data))
// #endif
//             {
//                break;
//             }
//             if(type == state->synth->_uAtom_Object)
//             {
//                const LV2_Atom_Object_Body *aObjBody = reinterpret_cast<LV2_Atom_Object_Body *>(data);
//                if(aObjBody->otype == state->synth->_uAtom_StateChanged)
//                {
//                   //Just make song status dirty (pending event) - something had changed in the plugin controls
//                   state->songDirtyPending = true;
//                }
//             }
//             if(state->uiInst == nullptr)
//             {
//                continue;
//             }
//             unsigned char atom_data [fifoItemSize];
//             LV2_Atom *atom_evt = reinterpret_cast<LV2_Atom *>(atom_data);
//             atom_evt->type = type;
//             atom_evt->size = size;
//             if(fifoItemSize - sizeof(LV2_Atom) < size)
//             {
// #ifdef DEBUG_LV2
//                std::cerr << "LV2Synth::lv2audio_postProcessMidiPorts(): Plugin event data is bigger than rt fifo item size. Skipping." << std::endl;
// #endif
//                continue;
//             }
//             unsigned char *evt = static_cast<unsigned char *>(LV2_ATOM_BODY(atom_evt));
//             memcpy(evt, data, size);
// 
//             state->plugControlEvt.put(state->midiOutPorts [j].index, sizeof(LV2_Atom) + size, atom_evt);
//          }
//          while(true);
// 
//       }
//    }
// 
// }
// 
// void LV2Synth::lv2ui_PostShow(LV2PluginWrapper_State *state)
// {
//    assert(state->pluginWindow != nullptr);
//    assert(state->uiDesc != nullptr);
//    assert(state->uiInst != nullptr);
// 
//    if(state->uiDesc->port_event != nullptr)
//    {
//       uint32_t numControls = 0;
//       Port *controls = nullptr;
// 
//       if(state->plugInst != nullptr)
//       {
//          numControls = state->plugInst->controlPorts;
//          controls = state->plugInst->controls;
// 
//       }
//       else if(state->sif != nullptr)
//       {
//          numControls = state->sif->_inportsControl;
//          controls = state->sif->_controls;
//       }
// 
//       if(numControls > 0)
//       {
//          assert(controls != nullptr);
//       }
// 
// 
// 
//       for(uint32_t i = 0; i < numControls; ++i)
//       {
//          state->uiDesc->port_event(state->uiInst,
//                                    controls [i].idx,
//                                    sizeof(float), 0,
//                                    &controls [i].val);
//       }
// 
//    }
// 
//    // Set the flag to tell the update timer to force sending all controls and program.
//    state->uiIsOpening = true;
//    
//    state->pluginWindow->startNextTime();
// 
// }

// TODO: UI
// int LV2Synth::lv2ui_Resize(LV2UI_Feature_Handle handle, int width, int height)
// {
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
//    if(state->widget != nullptr && state->hasGui)
//    {
//       ((LV2PluginWrapper_Window *)state->widget)->resize(width, height);
//       QWidget *ewWin = ((LV2PluginWrapper_Window *)state->widget)->findChild<QWidget *>();
//       if(ewWin != nullptr)
//       {
//          ewWin->resize(width, height);
//       }
//       else
//       {
// #ifdef LV2_GUI_USE_QWIDGET
//          // TODO Check this, maybe wrong widget, maybe need the one contained by it?
//          QWidget *ewCent= ((LV2PluginWrapper_Window *)state->widget);
// #else
//          QWidget *ewCent= ((LV2PluginWrapper_Window *)state->widget)->centralWidget();
// #endif
//          if(ewCent != nullptr)
//          {
//             ewCent->resize(width, height);
//          }
//       }
//       state->uiX11Size.setWidth(width);
//       state->uiX11Size.setHeight(height);
//       return 0;
//    }
//    return 1;
// }

// void LV2Synth::lv2ui_Gtk2AllocateCb(int width, int height, void *arg)
// {
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)arg;
//    if(state == nullptr)
//       return;
//    if(!state->gtk2AllocateCompleted && state->widget != nullptr && state->hasGui && state->gtk2Plug != nullptr)
//    {
//       state->gtk2AllocateCompleted = true;
//       ((LV2PluginWrapper_Window *)state->widget)->setMinimumSize(width, height);
//    }
// }
// 
// void LV2Synth::lv2ui_Gtk2ResizeCb(int width, int height, void *arg)
// {
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)arg;
//    if(state == nullptr)
//       return;
//    if(!state->gtk2ResizeCompleted && state->widget != nullptr && state->hasGui && state->gtk2Plug != nullptr)
//    {
//       state->gtk2ResizeCompleted = true;
//       ((LV2PluginWrapper_Window *)state->widget)->resize(width, height);
//    }
// }
// 
// void LV2Synth::lv2ui_ShowNativeGui(LV2PluginWrapper_State *state, bool bShow)
// {
//    LV2Synth* synth = state->synth;
//    LV2PluginWrapper_Window *win = nullptr;
// 
//    if(synth->_pluginUiTypes.size() == 0)
//       return;
// 
//    //state->uiTimer->stopNextTime();
//    if(state->pluginWindow != nullptr)
//       state->pluginWindow->stopNextTime();
// 
//    if(!bShow)
//       return;
//    
//    LV2_PLUGIN_UI_TYPES::iterator itUi;
// 
//    if((state->uiCurrent == nullptr) || MusEGlobal::config.lv2UiBehavior == MusEGlobal::CONF_LV2_UI_ASK_ALWAYS)
//    {
//       state->uiCurrent = nullptr;
//       state->gtk2ResizeCompleted = false;
//       state->gtk2AllocateCompleted = false;
//       QAction *aUiTypeSelected = nullptr;
//       if((synth->_pluginUiTypes.size() == 1) || MusEGlobal::config.lv2UiBehavior == MusEGlobal::CONF_LV2_UI_USE_FIRST)
//       {
//         state->uiCurrent = synth->_pluginUiTypes.begin()->first;
//       }
//       else
//       {
//          QMenu mGuisPopup;
//          MusEGui::MenuTitleItem *aUiTypeHeader = new MusEGui::MenuTitleItem(QObject::tr("Select gui type"), nullptr);
//          aUiTypeHeader->setEnabled(false);
//          QFont fHeader;
//          fHeader.setBold(true);
//          fHeader.setUnderline(true);
//          aUiTypeHeader->setFont(fHeader);
//          mGuisPopup.addAction(aUiTypeHeader);
// 
//          for(itUi = synth->_pluginUiTypes.begin(); itUi != synth->_pluginUiTypes.end(); itUi++)
//          {
//             const LilvUI *selectedUi = itUi->first;
//             const LilvNode *pluginUiType = itUi->second.second;
//             QAction *aUiType = new QAction(QString(lilv_node_as_string(pluginUiType)), nullptr);
//             aUiType->setData(QVariant(reinterpret_cast<qlonglong>(selectedUi)));
//             mGuisPopup.addAction(aUiType);
//          }
// 
//          aUiTypeSelected = mGuisPopup.exec(QCursor::pos());
//          if(aUiTypeSelected == nullptr)
//          {
//             return;
//          }
//          state->uiCurrent = reinterpret_cast<const LilvUI *>(aUiTypeSelected->data().toLongLong());
//       }
// 
//    }
// 
//    itUi = synth->_pluginUiTypes.find(state->uiCurrent);
// 
//    assert(itUi != synth->_pluginUiTypes.end());
// 
// 
//    const LilvUI *selectedUi = itUi->first;
//    bool bExtUi = itUi->second.first;
//    const LilvNode *pluginUiType = itUi->second.second;
//    state->uiIdleIface = nullptr;
//    if(bExtUi)
//    {
//       state->hasGui = false;
//       state->hasExternalGui = true;
//    }
//    else
//    {
//       state->hasGui = true;
//       state->hasExternalGui = false;
//    }
// 
// #ifdef LV2_GUI_USE_QWIDGET
//    win = new LV2PluginWrapper_Window(state, Q_NULLPTR, Qt::Window);
// #else
//    win = new LV2PluginWrapper_Window(state);
// #endif
//    
//    state->uiX11Size.setWidth(0);
//    state->uiX11Size.setHeight(0);
// 
//    if(win != nullptr)
//    {
//       state->widget = win;
//       state->pluginWindow = win;
//       const char *cUiUri = lilv_node_as_uri(pluginUiType);
//       const char *cUiTypeUri = lilv_node_as_uri(lilv_ui_get_uri(selectedUi));
//       bool bEmbed = false;
//       bool bGtk = false;
//       QWidget *ewWin = nullptr;
// #ifdef HAVE_GTK2
//       QWindow *x11QtWindow = nullptr;
// #endif
//       state->gtk2Plug = nullptr;
//       state->_ifeatures [synth->_fUiParent].data = nullptr;
//       if(strcmp(LV2_UI__X11UI, cUiUri) == 0)
//       {
//          bEmbed = true;         
//          ewWin = new QWidget();
//          
// #ifdef LV2_GUI_USE_QWIDGET
//          QVBoxLayout* layout = new QVBoxLayout();
//          layout->setMargin(0);
//          layout->setSpacing(0);
//          layout->addWidget(ewWin);
//          win->setLayout(layout);
//          
// #else
//          win->setCentralWidget(ewWin);
// #endif
//    
//          state->_ifeatures [synth->_fUiParent].data = (void*)ewWin->winId();
//       }
//       else if(strcmp(LV2_UI__GtkUI, cUiUri) == 0)
//       {
// #ifndef HAVE_GTK2
//          win->stopNextTime();
//          return;
// #else         
//          
//          bEmbed = true;
//          bGtk = true;
//          
// #ifdef LV2_GUI_USE_GTKPLUG
//          state->gtk2Plug = MusEGui::lv2Gtk2Helper_gtk_plug_new(0, state);
// #else
//          state->gtk2Plug = MusEGui::lv2Gtk2Helper_gtk_window_new(state);
// #endif
//          
//          MusEGui::lv2Gtk2Helper_register_allocate_cb(static_cast<void *>(state->gtk2Plug), lv2ui_Gtk2AllocateCb);
//          MusEGui::lv2Gtk2Helper_register_resize_cb(static_cast<void *>(state->gtk2Plug), lv2ui_Gtk2ResizeCb);
//          
// #endif         
//          
//       }
//       else if(strcmp(LV2_F_UI_Qt5_UI, cUiUri) == 0) //Qt5 uis are handled natively
//       {
//          state->_ifeatures [synth->_fUiParent].data = win;
//       }      
//       else //external uis
//       {
//          state->_ifeatures [synth->_fUiParent].data = nullptr;
//       }
// 
//       //now open ui library file
// 
//       // lilv_uri_to_path is deprecated. Use lilv_file_uri_parse instead. Return value must be freed with lilv_free.
//       const  char *uiPath = lilv_file_uri_parse(lilv_node_as_uri(lilv_ui_get_binary_uri(selectedUi)), nullptr);
// // REMOVE Tim. LV2. Changed. TESTING. RESTORE. Qt4 versions of synthv1,drumk,? crashes on Qt5.
// // TESTED: On my system it gets much farther into the call now, dozens of Qt4 calls into it, 
// //          but ultimately still ends up crashing on a call to dlopen libkdecore.5 for some reason.
// //       state->uiDlHandle = dlopen(uiPath, RTLD_NOW);
//       //state->uiDlHandle = dlmopen(LM_ID_NEWLM, uiPath, RTLD_LAZY | RTLD_DEEPBIND); // Just a test
//       state->uiDlHandle = dlopen(uiPath, RTLD_NOW | RTLD_DEEPBIND);
//       
//       lilv_free((void*)uiPath); // Must free.
//       if(state->uiDlHandle == nullptr)
//       {
//          win->stopNextTime();
//          return;
//       }
// 
//       //find lv2 ui descriptor function and call it to get ui descriptor struct
//       LV2UI_DescriptorFunction lv2fUiDesc;
//       *(void **)(&lv2fUiDesc) = dlsym(state->uiDlHandle, "lv2ui_descriptor");
//       if(lv2fUiDesc == nullptr)
//       {
//          win->stopNextTime();
//          return;
//       }
// 
//       state->uiDesc = nullptr;
// 
//       for(uint32_t i = 0; ;++i)
//       {
//          state->uiDesc = lv2fUiDesc(i);
//          if(state->uiDesc == nullptr)
//             break;
// 
//          if(strcmp(state->uiDesc->URI, cUiTypeUri) == 0) //found selected ui
//             break;
//       }
// 
//       if(state->uiDesc == nullptr)
//       {
//          win->stopNextTime();
//          return;
//       }
// 
//       void *uiW = nullptr;
//       // lilv_uri_to_path is deprecated. Use lilv_file_uri_parse instead. Return value must be freed with lilv_free.
//       const char* bundle_path = lilv_file_uri_parse(lilv_node_as_uri(lilv_ui_get_bundle_uri(selectedUi)), nullptr);
//       state->uiInst = state->uiDesc->instantiate(state->uiDesc,
//                                                  lilv_node_as_uri(lilv_plugin_get_uri(synth->_handle)),
//                                                  bundle_path,
//                                                  LV2Synth::lv2ui_PortWrite,
//                                                  state,
//                                                  &uiW,
//                                                  state->_ppifeatures);
// 
// 
//       lilv_free((void*)bundle_path); // Must free.
//       
//       if(state->uiInst != nullptr)
//       {
//          state->uiIdleIface = nullptr;
//          state->uiPrgIface = nullptr;
//          if(state->uiDesc->extension_data != nullptr)
//          {
//             state->uiIdleIface = (LV2UI_Idle_Interface *)state->uiDesc->extension_data(LV2_F_UI_IDLE);
//             state->uiPrgIface = (LV2_Programs_UI_Interface *)state->uiDesc->extension_data(LV2_PROGRAMSNEW__UIInterface);
//             if(state->uiPrgIface != nullptr)
//             {
//                state->newPrgIface = true;
//             }
//             else
//             {
//                state->newPrgIface = false;
//                state->uiPrgIface = (LV2_Programs_UI_Interface *)state->uiDesc->extension_data(LV2_PROGRAMS__UIInterface);
//             }
//          }
//          if(state->hasGui)
//          {            
//             if(!bEmbed)
//             {
// #ifdef LV2_GUI_USE_QWIDGET
//               QVBoxLayout* layout = new QVBoxLayout();
//               layout->setMargin(0);
//               layout->setSpacing(0);
//               layout->addWidget(static_cast<QWidget *>(uiW));
//               win->setLayout(layout);
// #else
//               win->setCentralWidget(static_cast<QWidget *>(uiW));
// #endif
//             }
//             else
//             {               
//                if(bGtk)
//                {
//                    
// #ifdef HAVE_GTK2
//                   MusEGui::lv2Gtk2Helper_gtk_container_add(state->gtk2Plug, uiW);
//                   MusEGui::lv2Gtk2Helper_gtk_widget_show_all(state->gtk2Plug);
//                   
// #ifdef LV2_GUI_USE_GTKPLUG
//                   unsigned long plugX11Id = MusEGui::lv2Gtk2Helper_gdk_x11_drawable_get_xid(state->gtk2Plug);
// #else
//                   unsigned long plugX11Id = MusEGui::lv2Gtk2Helper_gtk_window_get_xid(state->gtk2Plug);
// #endif                  
//                   
//                   x11QtWindow = QWindow::fromWinId(plugX11Id);
//                   ewWin = QWidget::createWindowContainer(x11QtWindow, win);
//                   state->pluginQWindow = x11QtWindow;
// 
// #ifdef LV2_GUI_USE_QWIDGET
//                   QVBoxLayout* layout = new QVBoxLayout();
//                   layout->setMargin(0);
//                   layout->setSpacing(0);
//                   layout->addWidget(ewWin);
//                   win->setLayout(layout);
// #else
//                   win->setCentralWidget(ewWin);
// #endif                  
// 
//                   if(state->uiX11Size.width() == 0 || state->uiX11Size.height() == 0)
//                   {
//                      int w = 0;
//                      int h = 0;
//                      MusEGui::lv2Gtk2Helper_gtk_widget_get_allocation(uiW, &w, &h);
//                      win->setMinimumSize(w, h);
//                      win->resize(w, h);
//                   }
// #endif                  
//                }
//                else
//                {
//                   if(state->uiX11Size.width() == 0 || state->uiX11Size.height() == 0)
//                      win->resize(ewWin->size());
//                }
//             }
// 
//             win->show();
//             win->setWindowTitle(state->extHost.plugin_human_id);
//          }
//          else if(state->hasExternalGui)
//          {
//             state->widget = uiW;
//             LV2_EXTERNAL_UI_SHOW((LV2_External_UI_Widget *)state->widget);
//          }
// 
//          LV2Synth::lv2ui_PostShow(state);
//          return;
// 
//       }
// 
//    }
//    if(win != nullptr)
//    {
//       win->stopNextTime();
//    }
//    state->pluginWindow = nullptr;
//    state->widget = nullptr;
//    state->uiCurrent = nullptr;
// 
//    //no ui is shown
//    state->hasExternalGui = state->hasGui = false;
// 
// }
// 
// 
// const void *LV2Synth::lv2state_stateRetreive(LV2_State_Handle handle, uint32_t key, size_t *size, uint32_t *type, uint32_t *flags)
// {
//    QMap<QString, QPair<QString, QVariant> >::const_iterator it;
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
//    LV2Synth *synth = state->synth;
//    const char *cKey = synth->unmapUrid(key);
// 
//    assert(cKey != nullptr); //should'n happen
// 
//    QString strKey = QString(cKey);
//    it = state->iStateValues.find(strKey);
//    if(it != state->iStateValues.end())
//    {
//       if(it.value().second.type() == QVariant::ByteArray)
//       {
//          QString sType = it.value().first;         
//          QByteArray arrType = sType.toUtf8();
//          *type = synth->mapUrid(arrType.constData());
//          *flags = LV2_STATE_IS_POD;
//          QByteArray valArr = it.value().second.toByteArray();
//          if(sType.compare(QString(LV2_ATOM__Path)) == 0) //prepend project folder to abstract path
//          {
//             QString plugFolder = ((state->sif != nullptr) ? state->sif->name() : state->plugInst->name()) + QString("/");
//             QString strPath = QString::fromUtf8(valArr.data());
//             QFile ff(strPath);
//             QFileInfo fiPath(ff);
//             if(fiPath.isRelative())
//             {
//                if(strPath.indexOf(plugFolder) < 0)
//                {
//                   strPath = plugFolder + strPath;
//                }
//                strPath = MusEGlobal::museProject + QString("/") + strPath;
//                valArr = strPath.toUtf8();
//                int len = strPath.length();
//                valArr.setRawData(strPath.toUtf8().constData(), len + 1);
//                valArr [len] = 0;
//             }
//          }
//          size_t i;
//          size_t numValues = state->numStateValues;
//          for(i = 0; i < numValues; ++i)
//          {
//             if(state->tmpValues [i] == nullptr)
//                break;
//          }
//          assert(i < numValues); //sanity check
//          size_t sz = valArr.size();
//          state->iStateValues.remove(strKey);
//          if(sz > 0)
//          {
//             state->tmpValues [i] = new char [sz];
//             memset(state->tmpValues [i], 0, sz);
//             memcpy(state->tmpValues [i], valArr.constData(), sz);
//             *size = sz;
//             return state->tmpValues [i];
//          }
//       }
//    }
// 
//    return NULL;
// }
// 
// LV2_State_Status LV2Synth::lv2state_stateStore(LV2_State_Handle handle, uint32_t key, const void *value, size_t size, uint32_t type, uint32_t flags)
// {
//    if(flags & (LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE))
//    {
//       LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
//       LV2Synth *synth = state->synth;
//       const char *uriKey = synth->unmapUrid(key);
//       const char *uriType = synth->unmapUrid(type);
//       assert(uriType != nullptr && uriKey != nullptr); //FIXME: buggy plugin or uridBiMap realization?
//       QString strKey = QString(uriKey);
//       QMap<QString, QPair<QString, QVariant> >::const_iterator it = state->iStateValues.find(strKey);
//       if(it == state->iStateValues.end())
//       {
//          QString strUriType = uriType;
//          QVariant varVal = QByteArray((const char *)value, size);
//          state->iStateValues.insert(strKey, QPair<QString, QVariant>(strUriType, varVal));
//       }
//       return LV2_STATE_SUCCESS;
//    }
//    return LV2_STATE_ERR_BAD_FLAGS;
// }

LV2_Worker_Status Lv2Plugin::lv2wrk_scheduleWork(LV2_Worker_Schedule_Handle handle, uint32_t size, const void *data)
{
#ifdef DEBUG_LV2
   std::cerr << "LV2Synth::lv2wrk_scheduleWork" << std::endl;
#endif
   LV2PluginWrapper_State *state = static_cast<LV2PluginWrapper_State*>(handle);

   //assert(state->wrkEndWork != true);
   if(state->wrkEndWork != false)
      return LV2_WORKER_ERR_UNKNOWN;

   state->wrkDataSize = size;
   state->wrkDataBuffer = data;
   if(MusEGlobal::audio->freewheel()) //don't wait for a thread. Do it now
      state->wrkThread->makeWork();
   else
      return state->wrkThread->scheduleWork();

   return LV2_WORKER_SUCCESS;
}

LV2_Worker_Status Lv2Plugin::lv2wrk_respond(LV2_Worker_Respond_Handle handle, uint32_t size, const void *data)
{
   LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;

   state->wrkDataSize = size;
   state->wrkDataBuffer = data;
   state->wrkEndWork = true;

   return LV2_WORKER_SUCCESS;
}

// void LV2Synth::lv2conf_write(LV2PluginWrapper_State *state, int level, Xml &xml)
// {
//    state->iStateValues.clear();
//    state->numStateValues = 0;
// 
//    if(state->iState != nullptr)
//    {
//       state->iState->save(lilv_instance_get_handle(state->handle), LV2Synth::lv2state_stateStore, state, LV2_STATE_IS_POD, state->_ppifeatures);
//    }
// 
//    if(state->sif != nullptr) // write control ports values only for synths
//    {
//       for(size_t c = 0; c < state->sif->_inportsControl; c++)
//       {
//          state->iStateValues.insert(QString(state->sif->_controlInPorts [c].cName), QPair<QString, QVariant>(QString(""), QVariant((double)state->sif->_controls[c].val)));
//       }
//    }
// 
//    if(state->uiCurrent != nullptr)
//    {
//       const char *cUiUri = lilv_node_as_uri(lilv_ui_get_uri(state->uiCurrent));
//       state->iStateValues.insert(QString(cUiUri), QPair<QString, QVariant>(QString(""), QVariant(QString(cUiUri))));
//    }
// 
//    QByteArray arrOut;
//    QDataStream streamOut(&arrOut, QIODevice::WriteOnly);
//    streamOut << state->iStateValues;
//    QByteArray outEnc64 = arrOut.toBase64();
//    QString customData(outEnc64);
//    for (int pos=0; pos < customData.size(); pos+=150)
//    {
//         customData.insert(pos++,'\n'); // add newlines for readability
//    }
//    xml.strTag(level, "customData", customData);
// }
// 
// void LV2Synth::lv2conf_set(LV2PluginWrapper_State *state, const std::vector<QString> &customParams)
// {
//    if(customParams.size() == 0)
//       return;
// 
//    state->iStateValues.clear();
//    for(size_t i = 0; i < customParams.size(); i++)
//    {
//       QString param = customParams [i];
//       param.remove('\n'); // remove all linebreaks that may have been added to prettyprint the songs file
//       QByteArray paramIn;
//       paramIn.append(param);
//       QByteArray dec64 = QByteArray::fromBase64(paramIn);
//       QDataStream streamIn(&dec64, QIODevice::ReadOnly);
//       streamIn >> state->iStateValues;
//       break; //one customData tag includes all data in base64
//    }
// 
//    size_t numValues = state->iStateValues.size();
//    state->numStateValues = numValues;
//    if(state->iState != nullptr && numValues > 0)
//    {
//       state->tmpValues = new char*[numValues];;
//       memset(state->tmpValues, 0, numValues * sizeof(char *));
//       state->iState->restore(lilv_instance_get_handle(state->handle), LV2Synth::lv2state_stateRetreive, state, 0, state->_ppifeatures);
//       for(size_t i = 0; i < numValues; ++i)
//       {
//          if(state->tmpValues [i] != nullptr)
//             delete [] state->tmpValues [i];
//       }
//       delete [] state->tmpValues;
//       state->tmpValues = nullptr;
//    }
// 
//    QMap<QString, QPair<QString, QVariant> >::const_iterator it;
//    for(it = state->iStateValues.begin(); it != state->iStateValues.end(); ++it)
//    {
//       QString name = it.key();
//       QVariant qVal = it.value().second;
//       if(!name.isEmpty() && qVal.isValid())
//       {
//          if(qVal.type() == QVariant::String) // plugin ui uri
//          {
//             QString sUiUri = qVal.toString();
//             LV2_PLUGIN_UI_TYPES::iterator it;
//             for(it = state->synth->_pluginUiTypes.begin(); it != state->synth->_pluginUiTypes.end(); ++it)
//             {
//                if(sUiUri == QString(lilv_node_as_uri(lilv_ui_get_uri(it->first))))
//                {
//                   state->uiCurrent = it->first;
//                   break;
//                }
//             }
//          }
//          else
//          {
//             if(state->sif != nullptr) //setting control value only for synths
//             {
// 
//                bool ok = false;
//                float val = (float)qVal.toDouble(&ok);
//                if(ok)
//                {
//                   std::map<QString, size_t>::iterator it = state->controlsNameMap.find(name.toLower());
//                   if(it != state->controlsNameMap.end())
//                   {
//                      size_t ctrlNum = it->second;
//                      state->sif->_controls [ctrlNum].val = state->sif->_controls [ctrlNum].tmpVal = val;
// 
//                   }
//                }
//             }
//          }
// 
// 
//       }
//    }
// 
// }

unsigned Lv2Plugin::lv2ui_IsSupported(const char *, const char *ui_type_uri)
{
   if(strcmp(LV2_F_UI_Qt5_UI, ui_type_uri) == 0
      || (strcmp(LV2_UI__GtkUI, ui_type_uri) == 0)
      || strcmp(LV2_UI__X11UI, ui_type_uri) == 0)
   {
      return 1;
   }
   return 0;
}

// void LV2Synth::lv2prg_updatePrograms(LV2PluginWrapper_State *state)
// {
//    assert(state != nullptr);
//    state->index2prg.clear();
//    state->prg2index.clear();
//    if(state->prgIface != nullptr)
//    {
//       uint32_t iPrg = 0;
//       const LV2_Program_Descriptor *pDescr = nullptr;
//       while((pDescr = state->prgIface->get_program(
//                 lilv_instance_get_handle(state->handle), iPrg)) != nullptr)
//       {
//         // 16384 banks arranged as 128 hi and lo banks each with up to the first 128 programs supported.
//         uint32_t hb = pDescr->bank >> 8;
//         uint32_t lb = pDescr->bank & 0xff;
//         if(hb < 128 && lb < 128 && pDescr->program < 128)
//         {
//          lv2ExtProgram extPrg;
//          extPrg.index = iPrg;
//          extPrg.bank = pDescr->bank;
//          extPrg.prog = pDescr->program;
//          extPrg.useIndex = true;
//          extPrg.name = QString(pDescr->name);
// 
//          state->index2prg.insert(std::make_pair(iPrg, extPrg));
//          hb &= 0x7f;
//          lb &= 0x7f;
//          uint32_t midiprg = (hb << 16) + (lb << 8) + extPrg.prog;
//          state->prg2index.insert(std::make_pair(midiprg, iPrg));
//         }
//          ++iPrg;
//       }
//    }
// 
// }

int Lv2Plugin::lv2_printf(LV2_Log_Handle handle, LV2_URID type, const char *fmt, ...)
{
   va_list argptr;
   va_start(argptr, fmt);
   int ret = lv2_vprintf(handle, type, fmt, argptr);
   va_end(argptr);
   return ret;
}

int Lv2Plugin::lv2_vprintf(LV2_Log_Handle, LV2_URID, const char *fmt, va_list ap)
{
   return vprintf(fmt, ap);

}

char *Lv2Plugin::lv2state_makePath(LV2_State_Make_Path_Handle handle, const char *path)
{
   LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
   assert(state != nullptr);

   QFile ff(path);
   QFileInfo fiPath(ff);

   if(fiPath.isAbsolute())
   {
      return strdup(path);
   }

   QString plugName = (state->sif != nullptr) ? state->sif->name() : state->plugInst->name();
   
  //-------------------------------------------------
  // TODO: Get rid of global! Keep a local version.   
  //-------------------------------------------------
   QString dirName = MusEGlobal::museProject + QString("/") + plugName;
   
   QDir dir;
   dir.mkpath(dirName);

   QString resPath = dirName + QString("/") + QString(path);
   return strdup(resPath.toUtf8().constData());

}

char *Lv2Plugin::lv2state_abstractPath(LV2_State_Map_Path_Handle handle, const char *absolute_path)
{
   LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
   assert(state != nullptr);

   //some plugins do not support abstract paths properly,
   //so return duplicate without modification for now
      //return strdup(absolute_path);

   QString resPath = QString(absolute_path);
   int rIdx = resPath.lastIndexOf('/');
   if(rIdx > -1)
   {
      resPath = resPath.mid(rIdx + 1);
   }
   QString plugName = (state->sif != nullptr) ? state->sif->name() : state->plugInst->name();
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

char *Lv2Plugin::lv2state_absolutePath(LV2_State_Map_Path_Handle handle, const char *abstract_path)
{
   return lv2state_makePath((LV2_State_Make_Path_Handle)handle, abstract_path);
}

// void LV2Synth::lv2state_populatePresetsMenu(LV2PluginWrapper_State *state, MusEGui::PopupMenu *menu)
// {
//    menu->clear();
//    menu->setIcon(QIcon(*MusEGui::presetsNewIcon));
//    LV2Synth *synth = state->synth;
//    //this is good by slow down menu population.
//    //So it's called only on changes (preset save/manual update)
//    //LV2Synth::lv2state_UnloadLoadPresets(synth, true);
//    MusEGui::MenuTitleItem *actPresetActionsHeader = new MusEGui::MenuTitleItem(QObject::tr("Preset actions"), menu);
//    menu->addAction(actPresetActionsHeader);
//    QAction *actSave = menu->addAction(QObject::tr("Save preset..."));
//    actSave->setObjectName("lv2state_presets_save_action");
//    actSave->setData(QVariant::fromValue<void *>(static_cast<void *>(lv2CacheNodes.lv2_actionSavePreset)));
//    QAction *actUpdate = menu->addAction(QObject::tr("Update list"));
//    actUpdate->setObjectName("lv2state_presets_update_action");
//    actUpdate->setData(QVariant::fromValue<void *>(static_cast<void *>(lv2CacheNodes.lv2_actionUpdatePresets)));
//    std::map<QString, LilvNode *>::iterator it;
//    MusEGui::MenuTitleItem *actSavedPresetsHeader = new MusEGui::MenuTitleItem(QObject::tr("Saved presets"), menu);
//    menu->addAction(actSavedPresetsHeader);
// 
//    for(it = synth->_presets.begin(); it != synth->_presets.end(); ++it)
//    {
//       QAction *act = menu->addAction(it->first);
//       act->setData(QVariant::fromValue<void *>(static_cast<void *>((it->second))));
//    }
//    if(menu->actions().size() == 0)
//    {
//       QAction *act = menu->addAction(QObject::tr("No presets found"));
//       act->setDisabled(true);
//       act->setData(QVariant::fromValue<void *>(NULL));
//    }   
// 
// 
// 
// }
// 
// void LV2Synth::lv2state_PortWrite(LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t protocol, const void *buffer, bool fromUi)
// {
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)controller;
// 
//    assert(state != nullptr); //this shouldn't happen
//    assert(state->inst != nullptr || state->sif != nullptr); // this too
// 
//    if(protocol != 0 && protocol != state->synth->_uAtom_EventTransfer)
//    {
// #ifdef DEBUG_LV2
//       std::cerr << "LV2Synth::lv2state_PortWrite: unsupported protocol (" << protocol << ")" << std::endl;
// #endif
//       return;
//    }
// 
//    if(protocol == state->synth->_uAtom_EventTransfer) //put atom transfers to dedicated ring buffer
//    {
// #ifdef DEBUG_LV2
//       std::cerr << "LV2Synth::lv2state_PortWrite: atom_EventTransfer, port = " << port_index << ", size =" << buffer_size << std::endl;
// #endif     
//       state->uiControlEvt.put(port_index, buffer_size, buffer);
//       return;
//    }
// 
//    std::map<uint32_t, uint32_t>::iterator it = state->synth->_idxToControlMap.find(port_index);
// 
//    if(it == state->synth->_idxToControlMap.end())
//    {
// #ifdef DEBUG_LV2
//       std::cerr << "LV2Synth::lv2state_PortWrite: wrong port index (" << port_index << ")" << std::endl;
// #endif
//       return;
//    }
// 
//    uint32_t cport = it->second;
//    float value = *(float *)buffer;
//    // Schedules a timed control change:
//    ControlEvent ce;
//    ce.unique = false;
//    ce.fromGui = fromUi;                 // It came from the plugin's own GUI (or not).
//    ce.idx = cport;
//    ce.value = value;
//    // Don't use timestamp(), because it's circular, which is making it impossible to deal
//    // with 'modulo' events which slip in 'under the wire' before processing the ring buffers.
//    ce.frame = MusEGlobal::audio->curFrame();
// 
//    ControlFifo *_controlFifo = nullptr;
//    if(state->inst != nullptr)
//    {
//       _controlFifo = &state->plugInst->_controlFifo;
//       if(fromUi)
//       {
//          // Record automation:
//          // Take care of this immediately, because we don't want the silly delay associated with
//          //  processing the fifo one-at-a-time in the apply().
//          // NOTE: With some vsts we don't receive control events until the user RELEASES a control.
//          // So the events all arrive at once when the user releases a control.
//          // That makes this pretty useless... But what the heck...
//          //AutomationType at = AUTO_OFF;
//          if(state->plugInst->_track && state->plugInst->_id != -1)
//          {
//             unsigned long id = genACnum(state->plugInst->_id, cport);
//             state->plugInst->_track->recordAutomation(id, value);
//             //at = state->plugInst->_track->automationType();
//          }
// 
//          //state->plugInst->enableController(cport, false);
//       }
//    }
//    else if(state->sif != nullptr)
//    {
//       _controlFifo = &state->sif->_controlFifo;
//       if(fromUi)
//       {
//          // Record automation:
//          // Take care of this immediately, because we don't want the silly delay associated with
//          //  processing the fifo one-at-a-time in the apply().
//          // NOTE: With some vsts we don't receive control events until the user RELEASES a control.
//          // So the events all arrive at once when the user releases a control.
//          // That makes this pretty useless... But what the heck...
//          if(state->sif->id() != -1)
//          {
//             unsigned long pid = genACnum(state->sif->id(), cport);
//             state->sif->synti->recordAutomation(pid, value);
//          }
// 
//          //state->sif->enableController(cport, false);
//       }
//    }
// 
//    if(fromUi)
//    {
//       state->controlTimers [cport] = 1000 / 30; //  1 sec controllers will not be send to guis
//    }
// 
//    assert(_controlFifo != nullptr);
//    if(_controlFifo->put(ce))
//      std::cerr << "LV2Synth::lv2state_PortWrite: fifo overflow: in control number:" << cport << std::endl;
// 
// #ifdef DEBUG_LV2
//    std::cerr << "LV2Synth::lv2state_PortWrite: port=" << cport << "(" << port_index << ")" << ", val=" << value << std::endl;
// #endif
// 
// 
// }
// 
// void LV2Synth::lv2state_setPortValue(const char *port_symbol, void *user_data, const void *value, uint32_t size, uint32_t type)
// {
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)user_data;
//    assert(state != nullptr);
//    std::map<QString, size_t>::iterator it = state->controlsSymMap.find(QString::fromUtf8(port_symbol).toLower());
//    if(it != state->controlsSymMap.end())
//    {
//       size_t ctrlNum = it->second;
//       uint32_t ctrlIdx = state->synth->_controlInPorts [ctrlNum].index;
//       float fvalue;
//       if (type == state->atomForge.Float)
//       {
//          fvalue = *(const float*)value;
//       }
//       else if (type == state->atomForge.Double)
//       {
//          fvalue = *(const double*)value;
//       }
//       else if (type == state->atomForge.Int)
//       {
//          fvalue = *(const int32_t*)value;
//       }
//       else if (type == state->atomForge.Long)
//       {
//          fvalue = *(const int64_t*)value;
//       }
//       else
//       {
//          fprintf(stderr, "error: Preset `%s' value has bad type <%s>\n",
//                  port_symbol, state->synth->uridBiMap.unmap(type));
//          return;
//       }
//       LV2Synth::lv2state_PortWrite((LV2UI_Controller)user_data, ctrlIdx, size, 0, &fvalue, false);
//    }
// 
// }
// 
// const void *LV2Synth::lv2state_getPortValue(const char *port_symbol, void *user_data, uint32_t *size, uint32_t *type)
// {
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)user_data;
//    assert(state != nullptr);
//    std::map<QString, size_t>::iterator it = state->controlsSymMap.find(QString::fromUtf8(port_symbol).toLower());
//    *size = *type = 0;
//    if(it != state->controlsSymMap.end())
//    {
//       size_t ctrlNum = it->second;
//       MusECore::Port *controls = nullptr;
// 
//       if(state->plugInst != nullptr)
//       {
//          controls = state->plugInst->controls;
// 
//       }
//       else if(state->sif != nullptr)
//       {
//          controls = state->sif->_controls;
//       }
// 
//       if(controls != nullptr)
//       {
//          *size = sizeof(float);
//          *type = state->atomForge.Float;
//          return &controls [ctrlNum].val;
//       }
//    }
// 
//    return NULL;
// 
// }
// 
// void LV2Synth::lv2state_applyPreset(LV2PluginWrapper_State *state, LilvNode *preset)
// {
//    //handle special actions first
//    if(preset == lv2CacheNodes.lv2_actionSavePreset)
//    {
//       bool isOk = false;
//       QString presetName = QInputDialog::getText(MusEGlobal::muse, QObject::tr("Enter new preset name"),
//                                                  QObject::tr(("Preset name:")), QLineEdit::Normal,
//                                                  QString(""), &isOk);
//       if(isOk && !presetName.isEmpty())
//       {
//          presetName = presetName.trimmed();
//          QString synthName = state->synth->name().replace(' ', '_');
//          QString presetDir = MusEGlobal::museUser + QString("/.lv2/")
//                              + synthName + QString("_")
//                              + presetName + QString(".lv2/");
//          QString presetFile = synthName + QString("_") + presetName
//                               + QString(".ttl");
//          QString plugName = (state->sif != nullptr) ? state->sif->name() : state->plugInst->name();
//          QString plugFileDirName = MusEGlobal::museProject + QString("/") + plugName;
//          char *cPresetName = strdup(presetName.toUtf8().constData());
//          char *cPresetDir = strdup(presetDir.toUtf8().constData());
//          char *cPresetFile = strdup(presetFile.toUtf8().constData());
//          char *cPlugFileDirName = strdup(plugFileDirName.toUtf8().constData());
//          LilvState* const lilvState = lilv_state_new_from_instance(state->synth->_handle, state->handle, &state->synth->_lv2_urid_map,
//                                                                    cPlugFileDirName, cPresetDir, cPresetDir, cPresetDir,
//                                                                    LV2Synth::lv2state_getPortValue, state,
//                                                                    LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE, nullptr);
// 
//          lilv_state_set_label(lilvState, cPresetName);
// 
//          lilv_state_save(lilvWorld, &state->synth->_lv2_urid_map,
//                          &state->synth->_lv2_urid_unmap,
//                          lilvState, nullptr, cPresetDir,
//                          cPresetFile);
// 
//          lilv_state_free(lilvState);
//          free(cPresetName);
//          free(cPresetDir);
//          free(cPresetFile);
//          free(cPlugFileDirName);
//          LV2Synth::lv2state_UnloadLoadPresets(state->synth, true, true);
// 
//       }
// 
//       return;
//    }
//    else if(preset == lv2CacheNodes.lv2_actionUpdatePresets)
//    {
//       LV2Synth::lv2state_UnloadLoadPresets(state->synth, true, true);
//       return;
//    }
//    LilvState* lilvState = lilv_state_new_from_world(lilvWorld, &state->synth->_lv2_urid_map, preset);
//    if(lilvState)
//    {
//       lilv_state_restore(lilvState, state->handle, LV2Synth::lv2state_setPortValue, state, 0, nullptr);
//       lilv_state_free(lilvState);
//    }
// 
// }

void Lv2Plugin::lv2state_UnloadLoadPresets(Lv2Plugin *synth, bool load, bool update)
{
   assert(synth != nullptr);

   //std::cerr << "Lv2Plugin::lv2state_UnloadLoadPresets:  handling <" << synth->_name.toStdString() << ">" << std::endl;

   std::map<QString, LilvNode *>::iterator it;
   for(it = synth->_presets.begin(); it != synth->_presets.end(); ++it)
   {
      lilv_world_unload_resource(lilvWorld, it->second);
      lilv_node_free(it->second);      
   }
   synth->_presets.clear();



   if(load)
   {
      if(update)
      {
         //rescan and refresh user-defined presets first
         QDirIterator dir_it(MusEGlobal::museUser + QString("/.lv2"), QStringList() << "*.lv2", QDir::Dirs, QDirIterator::NoIteratorFlags);
         while (dir_it.hasNext())
         {
            QString nextDir = dir_it.next() + QString("/");
            std::cerr << nextDir.toStdString() << std::endl;
            SerdNode  sdir = serd_node_new_file_uri((const uint8_t*)nextDir.toUtf8().constData(), 0, 0, 0);
            LilvNode* ldir = lilv_new_uri(lilvWorld, (const char*)sdir.buf);
            lilv_world_unload_bundle(lilvWorld, ldir);
            lilv_world_load_bundle(lilvWorld, ldir);
            serd_node_free(&sdir);
            lilv_node_free(ldir);
         }
      }

      //scan for preserts
      LilvNodes* presets = lilv_plugin_get_related(synth->_handle, lv2CacheNodes.lv2_psetPreset);
      LILV_FOREACH(nodes, i, presets)
      {
         const LilvNode* preset = lilv_nodes_get(presets, i);
#ifdef DEBUG_LV2
         std::cerr << "\tPreset: " << lilv_node_as_uri(preset) << std::endl;
#endif
         lilv_world_load_resource(lilvWorld, preset);
         LilvNodes* pLabels = lilv_world_find_nodes(lilvWorld, preset, lv2CacheNodes.lv2_rdfsLabel, nullptr);
         if (pLabels != nullptr)
         {
            const LilvNode* pLabel = lilv_nodes_get_first(pLabels);
            synth->_presets.insert(std::make_pair(lilv_node_as_string(pLabel), lilv_node_duplicate(preset)));
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

   }

}




// TODO: prg
// void LV2SynthIF::lv2prg_Changed(LV2_Programs_Handle handle, int32_t index)
// {
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
// #ifdef DEBUG_LV2
//    std::cerr << "LV2Synth::lv2prg_Changed: index: " << index << std::endl;
// #endif
//    if(state->sif && state->sif->synti)
//    {
//       std::map<uint32_t, lv2ExtProgram>::iterator itIndex = state->index2prg.find(index);
//       if(itIndex == state->index2prg.end())
//          return;
//       int ch      = 0;
//       int port    = state->sif->synti->midiPort();
//       const lv2ExtProgram &extPrg = itIndex->second;
//       uint32_t hb = extPrg.bank >> 8;
//       uint32_t lb = extPrg.bank & 0xff;
//       if(hb > 127 || lb > 127 || extPrg.prog > 127)
//         return;
//       hb &= 0x7f;
//       lb &= 0x7f;
//       state->sif->synti->setCurrentProg(ch, extPrg.prog, lb, hb);
//       const int rv = (hb << 16) | (lb << 8) | extPrg.prog;
//       if(port != -1)
//       {
//         MidiPlayEvent event(0, port, ch, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, rv);
//         //MusEGlobal::midiPorts[port].sendEvent(event);
//         MusEGlobal::midiPorts[port].sendHwCtrlState(event, false);
//         if(state->sif->id() != -1 && state->sif->_controls != nullptr)
//         {
//            for(unsigned long k = 0; k < state->sif->_inportsControl; ++k)
//            {
//               state->sif->synti->setPluginCtrlVal(genACnum(state->sif->id(), k), state->sif->_controls[k].val);
//            }
//         }
//       }
//    }
// }


// LV2Synth::LV2Synth(const QFileInfo &fi, QString label, QString name, QString author, const LilvPlugin *_plugin, Plugin::PluginFeatures reqFeatures)
//    : Synth(fi, label, name, author, QString(""), reqFeatures),
//Lv2Plugin::Lv2Plugin(const QFileInfo *fi, QString /*label*/, QString /*name*/, QString /*author*/, const LilvPlugin *plugin)
Lv2Plugin::Lv2Plugin(const QFileInfo *fi, QString label, QString name, QString author, 
                     const LilvPlugin *plugin, PluginFeaturesType requiredFeatures)
   : Plugin(fi),
     _handle(plugin),
     _features(NULL),
     _ppfeatures(NULL),
//      _options(NULL),
     _isSynth(false),
     _uis(NULL),
     _hasFreeWheelPort(false),
     _isConstructed(false),
     _pluginControlsDefault(NULL),
     _pluginControlsMin(NULL),
     _pluginControlsMax(NULL)
{
  _label = label;
  _name = name;
  _maker = author;
  _requiredFeatures = requiredFeatures;
     
   //fake id for LV2PluginWrapper functionality
   _uniqueID = uniqueID++;

   _midi_event_id = mapUrid(LV2_MIDI__MidiEvent);

   _uTime_Position        = mapUrid(LV2_TIME__Position);
   _uTime_frame           = mapUrid(LV2_TIME__frame);
   _uTime_speed           = mapUrid(LV2_TIME__speed);
   _uTime_beatsPerMinute  = mapUrid(LV2_TIME__beatsPerMinute);
   _uTime_barBeat         = mapUrid(LV2_TIME__barBeat);
   _uAtom_EventTransfer   = mapUrid(LV2_ATOM__eventTransfer);
   _uAtom_Chunk           = mapUrid(LV2_ATOM__Chunk);
   _uAtom_Sequence        = mapUrid(LV2_ATOM__Sequence);
   _uAtom_StateChanged    = mapUrid(LV2_F_STATE_CHANGED);
   _uAtom_Object          = mapUrid(LV2_ATOM__Object);

//    _sampleRate = (double)MusEGlobal::sampleRate;
//    _fSampleRate = (float)MusEGlobal::sampleRate;
//    _iSampleRate = samplerate;
//    _sampleRate = (double)_iSampleRate;
//    _fSampleRate = (float)_iSampleRate;
// 
//    _segSize = segment_size;
/*   
   //prepare features and options arrays
   LV2_Options_Option _tmpl_options [] =
   {
      {LV2_OPTIONS_INSTANCE, 0, uridBiMap.map(LV2_P_SAMPLE_RATE), sizeof(float), uridBiMap.map(LV2_ATOM__Float), &_fSampleRate},
      {LV2_OPTIONS_INSTANCE, 0, uridBiMap.map(LV2_P_MIN_BLKLEN), sizeof(int32_t), uridBiMap.map(LV2_ATOM__Int), &Lv2Plugin::minBlockSize}, //&_segSize},
      {LV2_OPTIONS_INSTANCE, 0, uridBiMap.map(LV2_P_MAX_BLKLEN), sizeof(int32_t), uridBiMap.map(LV2_ATOM__Int), &_segSize},
      {LV2_OPTIONS_INSTANCE, 0, uridBiMap.map(LV2_P_NOM_BLKLEN), sizeof(int32_t), uridBiMap.map(LV2_ATOM__Int), &_segSize},
      {LV2_OPTIONS_INSTANCE, 0, uridBiMap.map(LV2_P_SEQ_SIZE), sizeof(int32_t), uridBiMap.map(LV2_ATOM__Int), &_segSize},
      {LV2_OPTIONS_INSTANCE, 0, uridBiMap.map(LV2_CORE__sampleRate), sizeof(double), uridBiMap.map(LV2_ATOM__Double), &_sampleRate},
      {LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, nullptr}

   };

   _options = new LV2_Options_Option[SIZEOF_ARRAY(_tmpl_options)]; // last option is NULLs

   for(uint32_t i = 0; i < SIZEOF_ARRAY(_tmpl_options); i++)
   {
      _options [i] = _tmpl_options [i];
   }*/

   _features = new LV2_Feature[SIZEOF_ARRAY(lv2Features)];
   _ppfeatures = new LV2_Feature *[SIZEOF_ARRAY(lv2Features) + 1];
   _lv2_urid_map.map = Synth_Urid_Map;
   _lv2_urid_map.handle = this;
   _lv2_urid_unmap.unmap = Synth_Urid_Unmap;
   _lv2_urid_unmap.handle = this;
#ifdef LV2_URI_MAP_SUPPORT
   _lv2_uri_map.uri_to_id = Synth_Uri_Map;
   _lv2_uri_map.callback_data = this;
#endif
   _lv2_log_log.handle = this;
   _lv2_log_log.printf = lv2_printf;
   _lv2_log_log.vprintf = lv2_vprintf;

   uint32_t i;

   for(i = 0; i < SIZEOF_ARRAY(lv2Features); i++)
   {
      _features [i] = lv2Features [i];

      if(_features [i].URI == nullptr)
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
#ifdef LV2_URI_MAP_SUPPORT
      else if(std::string(LV2_F_URI_MAP) == _features [i].URI)
      {
         _features [i].data = &_lv2_uri_map;
      }
#endif
      else if(std::string(LV2_F_OPTIONS) == _features [i].URI)
      {
//          _features [i].data = _options;
         _fOptions = i;
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
        if(_nPname != 0)
          lilv_node_free(_nPname);
         continue;
      }

      bool isCVPort = lilv_port_is_a(_handle, _port, lv2CacheNodes.lv2_CVPort);

      if(lilv_port_is_a(_handle, _port, lv2CacheNodes.lv2_ControlPort) || isCVPort)
      {
         LV2ControlPortType _cType = LV2_PORT_CONTINUOUS;
         if(lilv_port_has_property(_handle, _port, lv2CacheNodes.lv2_portInteger))
            _cType = LV2_PORT_INTEGER;
         else if(lilv_port_has_property(_handle, _port, lv2CacheNodes.lv2_portTrigger)
                 || lilv_port_has_property(_handle, _port, lv2CacheNodes.lv2_portToggled))
            _cType = LV2_PORT_TRIGGER;
         else if(lilv_port_has_property(_handle, _port, lv2CacheNodes.lv2_portLogarithmic))
            _cType = LV2_PORT_LOGARITHMIC;

         cPorts->push_back(LV2ControlPort(_port, i, 0.0f, _portName, _portSym, _cType, isCVPort));

         if(std::isnan(_pluginControlsDefault [i]))
            _pluginControlsDefault [i] = 0;
         if(std::isnan(_pluginControlsMin [i]))
            _pluginControlsMin [i] = 0;
         if(std::isnan(_pluginControlsMax [i]))
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
         aPorts->push_back(LV2AudioPort(_port, i, nullptr, _portName));
      }
#ifdef LV2_EVENT_BUFFER_SUPPORT
      else if(lilv_port_is_a(_handle, _port, lv2CacheNodes.ev_EventPort))
      {
         bool portSupportsTimePos = lilv_port_supports_event(_handle, _port, lv2CacheNodes.lv2_TimePosition);
         mPorts->push_back(LV2MidiPort(_port, i, _portName, true /* old api is on */,portSupportsTimePos));
      }
#endif
      else if(lilv_port_is_a(_handle, _port, lv2CacheNodes.atom_AtomPort))
      {
         bool portSupportsTimePos = lilv_port_supports_event(_handle, _port, lv2CacheNodes.lv2_TimePosition);
#ifdef LV2_EVENT_BUFFER_SUPPORT
         mPorts->push_back(LV2MidiPort(_port, i, _portName, false /* old api is off */, portSupportsTimePos));
#else
         mPorts->push_back(LV2MidiPort(_port, i, _portName, portSupportsTimePos));
#endif
      }
      else if(!optional)
      {
//#ifdef DEBUG_LV2
         std::cerr << "plugin has port with unknown type - ignoring plugin " << label.toStdString() << "!" << std::endl;
//#endif
        if(_nPname != 0)
          lilv_node_free(_nPname);
         return;
      }

      if(_nPname != 0)
        lilv_node_free(_nPname);
   }

   for(uint32_t i = 0; i < _controlInPorts.size(); ++i)
   {
      _idxToControlMap.insert(std::pair<uint32_t, uint32_t>(_controlInPorts [i].index, i));
      if(lilvFreeWheelPort != nullptr)
      {
         if(lilv_port_get_index(_handle, _controlInPorts [i].port) == lilv_port_get_index(_handle, lilvFreeWheelPort))
         {
            _hasFreeWheelPort = true;
            _freeWheelPortIndex = i;
         }
      }
   }

   const LilvPluginClass *cls = lilv_plugin_get_class(_handle);
   const LilvNode *ncuri = lilv_plugin_class_get_uri(cls);
   const char *clsname = lilv_node_as_uri(ncuri);
   if((strcmp(clsname, LV2_INSTRUMENT_CLASS) == 0) && (_midiInPorts.size() > 0))
   {
      _isSynth = true;
   }

   const LilvNode *pluginUIType = nullptr;
   _uis = nullptr;


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
                                 lv2ui_IsSupported,
                                 lv2CacheNodes.host_uiType,
                                 &pluginUIType))
         {
#ifdef DEBUG_LV2
            const char *strUiType = lilv_node_as_string(pluginUIType); //internal uis are preferred
            std::cerr << "Plugin " << label.toStdString() << " supports ui of type " << strUiType << std::endl;
#endif

#ifndef HAVE_GTK2            
            const char *cUiUri = lilv_node_as_uri(pluginUIType);
            if(strcmp(LV2_UI__GtkUI, cUiUri) != 0)
#endif              
              
              _pluginUiTypes.insert(std::make_pair(ui, std::make_pair(false, pluginUIType)));
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
                  
#ifndef HAVE_GTK2            
                  const char *cUiUri = lilv_node_as_uri(pluginUIType);
                  if(strcmp(LV2_UI__GtkUI, cUiUri) != 0)
#endif              
              
                    _pluginUiTypes.insert(std::make_pair(ui, std::make_pair(true, pluginUIType)));
               }

               nit = lilv_nodes_next(nUiClss, nit);
            }

         }

         it = lilv_uis_next(_uis, it);
      };


   }

   _presets.clear();

   lv2state_UnloadLoadPresets(this, true);

   _isConstructed = true;
}

Lv2Plugin::~Lv2Plugin()
{
   lv2state_UnloadLoadPresets(this);

   if(_ppfeatures)
   {
      delete [] _ppfeatures;
      _ppfeatures = nullptr;
   }

   if(_features)
   {
      delete [] _features;
      _features = nullptr;
   }

//    if(_options)
//    {
//       delete [] _options;
//       _options = nullptr;
//    }

   if(_uis != nullptr)
   {
      lilv_uis_free(_uis);
      _uis = nullptr;
   }
}

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

void* Lv2Plugin::instantiate(float sampleRate, void* data)
{
// //   _sampleRate = sampleRate;
//   _iSampleRate = sampleRate;
//   _sampleRate = (double)_iSampleRate;
//   _fSampleRate = (float)_iSampleRate;
// 
//    _segSize = segment_size;
//   bool success = false;
//   LADSPA_Handle h = plugin->instantiate(plugin, _sampleRate);
//   success = (h != nullptr);
//   if (success)
//         SP_DBG_LADSPA2("Plugin instantiated", label().toLatin1().constData());
//   return h;
  
  
  
//    _synth = s;
// 
//    //use LV2Synth features as template
// 
//    _state = new LV2PluginWrapper_State;
//    _state->inst = nullptr;
//    _state->widget = nullptr;
//    _state->uiInst = nullptr;
//    _state->plugInst = nullptr;
//    _state->_ifeatures = new LV2_Feature[SIZEOF_ARRAY(lv2Features)];
//    _state->_ppifeatures = new LV2_Feature *[SIZEOF_ARRAY(lv2Features) + 1];
//    _state->sif = this;
//    _state->synth = _synth;
// 
//    LV2_Options_Option* options = (LV2_Options_Option*)data;
//    LV2_Feature *ifeatures = new LV2_Feature[SIZEOF_ARRAY(lv2Features)];
//    LV2_Feature **ppifeatures = new LV2_Feature *[SIZEOF_ARRAY(lv2Features) + 1];   
//    
//    //LV2Synth::lv2state_FillFeatures(_state);
//    //lv2state_FillFeatures(_state);
//    lv2_FillFeatures(ifeatures, ppifeatures, options);

   //_state->handle = _handle = lilv_plugin_instantiate(_synth->_handle, (double)MusEGlobal::sampleRate, _state->_ppifeatures);
   //LilvInstance* inst = lilv_plugin_instantiate(_handle, sampleRate, (LV2_Feature**)data);
  
   LV2PluginWrapper_State *state = static_cast<LV2PluginWrapper_State*>(data);
   LilvInstance* inst = lilv_plugin_instantiate(_handle, sampleRate, state->_ppifeatures);

//    if(_handle == nullptr)
//    {
//       delete [] _state->_ppifeatures;
//       _state->_ppifeatures = nullptr;
//       delete [] _state->_ifeatures;
//       _state->_ifeatures = nullptr;
//       return false;
//    }
//    if(inst == nullptr)
//    {
//       delete [] ppifeatures;
//       //ppifeatures = nullptr;
//       delete [] ifeatures;
//       //ifeatures = nullptr;
//       return NULL;
//    }

   if(inst)
   {
     SP_DBG_LV2_2("Plugin instantiated", label().toLatin1().constData());
   }
   
   return inst;
   
//    _audioInPorts = s->_audioInPorts;
//    _audioOutPorts = s->_audioOutPorts;
//    _controlInPorts = s->_controlInPorts;
//    _controlOutPorts = s->_controlOutPorts;
//    _inportsMidi = _state->midiInPorts.size();
//    _outportsMidi = _state->midiOutPorts.size();
// 
// 
// 
//    _inportsControl = _controlInPorts.size();
//    _outportsControl = _controlOutPorts.size();
// 
//    if(_inportsControl != 0)
//    {
//       _controls = new Port[_inportsControl];
//    }
//    else
//    {
//       _controls = nullptr;
//    }
// 
//    if(_outportsControl != 0)
//    {
//       _controlsOut = new Port[_outportsControl];
//    }
//    else
//    {
//       _controlsOut = nullptr;
//    }
// 
//    _synth->midiCtl2PortMap.clear();
//    _synth->port2MidiCtlMap.clear();
// 
//    for(size_t i = 0; i < _inportsControl; i++)
//    {
//       uint32_t idx = _controlInPorts [i].index;
//       _controls [i].idx = idx;
//       _controls [i].val = _controls [i].tmpVal = _controlInPorts [i].defVal = _synth->_pluginControlsDefault [idx];
//       if(_synth->_hasFreeWheelPort && _synth->_freeWheelPortIndex == i)
//          _controls [i].enCtrl = false;
//       else
//          _controls [i].enCtrl = true;
//       _controlInPorts [i].minVal = _synth->_pluginControlsMin [idx];
//       _controlInPorts [i].maxVal = _synth->_pluginControlsMax [idx];
// 
//       int ctlnum = CTRL_NRPN14_OFFSET + 0x2000 + i;
// 
//       // We have a controller number! Insert it and the lv2 port number into both maps.
//       _synth->midiCtl2PortMap.insert(std::pair<int, int>(ctlnum, i));
//       _synth->port2MidiCtlMap.insert(std::pair<int, int>(i, ctlnum));
// 
//       int id = genACnum(MAX_PLUGINS, i);
//       CtrlList *cl;
//       CtrlListList *cll = track()->controller();
//       iCtrlList icl = cll->find(id);
// 
//       if(icl == cll->end())
//       {
//          cl = new CtrlList(id);
//          cll->add(cl);
//          cl->setCurVal(_controls[i].val);
//       }
//       else
//       {
//          cl = icl->second;
//          _controls[i].val = cl->curVal();
//       }
// 
//       cl->setRange(_synth->_pluginControlsMin [idx], _synth->_pluginControlsMax [idx]);
//       cl->setName(QString(_controlInPorts [i].cName));
//       CtrlValueType vt = VAL_LINEAR;
//       switch(_controlInPorts [i].cType)
//       {
//       case LV2_PORT_CONTINUOUS:
//          vt = VAL_LINEAR;
//          break;
//       case LV2_PORT_DISCRETE:
//       case LV2_PORT_INTEGER:
//          vt = VAL_INT;
//          break;
//       case LV2_PORT_LOGARITHMIC:
//          vt = VAL_LOG;
//          break;
//       case LV2_PORT_TRIGGER:
//          vt = VAL_BOOL;
//          break;
//       default:
//          break;
//       }
// 
//       cl->setValueType(vt);
//       cl->setMode(((_controlInPorts [i].cType == LV2_PORT_CONTINUOUS)
//                    ||(_controlInPorts [i].cType == LV2_PORT_LOGARITHMIC))? CtrlList::INTERPOLATE : CtrlList::DISCRETE);
// 
//       if(!_controlInPorts [i].isCVPort)
//          lilv_instance_connect_port(_handle, idx, &_controls [i].val);
//    }
// 
//    for(size_t i = 0; i < _outportsControl; i++)
//    {
//       uint32_t idx = _controlOutPorts [i].index;
//       _controlsOut[i].idx = idx;
//       _controlsOut[i].val    = 0.0;
//       _controlsOut[i].tmpVal = 0.0;
//       _controlsOut[i].enCtrl  = false;
//       _controlOutPorts [i].defVal = _controlOutPorts [i].minVal = _controlOutPorts [i].maxVal = 0.0;
//       if(!_controlOutPorts [i].isCVPort)
//          lilv_instance_connect_port(_handle, idx, &_controlsOut[i].val);
//    }
// 
// 
//    int rv = posix_memalign((void **)&_audioInSilenceBuf, 16, sizeof(float) * MusEGlobal::segmentSize);
// 
//    if(rv != 0)
//    {
//       fprintf(stderr, "ERROR: LV2SynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
//       abort();
//    }
// 
//    if(MusEGlobal::config.useDenormalBias)
//    {
//       for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
//       {
//          _audioInSilenceBuf[q] = MusEGlobal::denormalBias;
//       }
//    }
//    else
//    {
//       memset(_audioInSilenceBuf, 0, sizeof(float) * MusEGlobal::segmentSize);
//    }
// 
//    //cache number of ports
//    _inports = _audioInPorts.size();
//    _outports = _audioOutPorts.size();
// 
// 
//    if(_inports > 0)
//    {
//       _audioInBuffers = new float*[_inports];
// 
//       for(size_t i = 0; i < _inports; i++)
//       {
//          int rv = posix_memalign((void **)&_audioInBuffers [i], 16, sizeof(float) * MusEGlobal::segmentSize);
// 
//          if(rv != 0)
//          {
//             fprintf(stderr, "ERROR: LV2SynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
//             abort();
//          }
// 
//          if(MusEGlobal::config.useDenormalBias)
//          {
//             for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
//             {
//                _audioInBuffers [i][q] = MusEGlobal::denormalBias;
//             }
//          }
//          else
//          {
//             memset(_audioInBuffers [i], 0, sizeof(float) * MusEGlobal::segmentSize);
//          }
//          
//          _audioInPorts [i].buffer = _audioInBuffers [i];
//          lilv_instance_connect_port(_handle, _audioInPorts [i].index, _audioInBuffers [i]);
//       }
//    }
// 
//    if(_outports > 0)
//    {
//       _audioOutBuffers = new float*[_outports];
// 
//       for(size_t i = 0; i < _outports; i++)
//       {
//          int rv = posix_memalign((void **)&_audioOutBuffers [i], 16, sizeof(float) * MusEGlobal::segmentSize);
// 
//          if(rv != 0)
//          {
//             fprintf(stderr, "ERROR: LV2SynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
//             abort();
//          }
// 
//          if(MusEGlobal::config.useDenormalBias)
//          {
//             for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
//             {
//                _audioOutBuffers [i][q] = MusEGlobal::denormalBias;
//             }
//          }
//          else
//          {
//             memset(_audioOutBuffers [i], 0, sizeof(float) * MusEGlobal::segmentSize);
//          }
// 
//          _audioOutPorts [i].buffer = _audioOutBuffers [i];
//          lilv_instance_connect_port(_handle, _audioOutPorts [i].index, _audioOutBuffers [i]);
//       }
//    }
// 
//    LV2Synth::lv2state_PostInstantiate(_state);
//    activate();
// 
// 
// 
//    return true;

  
}

//---------------------------------------------------------
//   incReferences
//---------------------------------------------------------

int Lv2Plugin::incReferences(int val)
{
  #ifdef PLUGIN_DEBUGIN
  fprintf(stderr, "Lv2Plugin::incReferences _references:%d val:%d\n", _references, val);
  #endif

  int newref = _references + val;

  if(newref <= 0)
  {
    _references = 0;
    if(_libHandle)
    {
      #ifdef PLUGIN_DEBUGIN
      //fprintf(stderr, "Lv2Plugin::incReferences no more instances, closing library\n");
      fprintf(stderr, "Lv2Plugin::incReferences Error: _libHandle is not NULL\n");
      #endif

      //dlclose(_libHandle);
    }

    _libHandle = 0;
    //_plugin = nullptr;
//     _pIdx.clear();
//     _poIdx.clear();
//     _iIdx.clear();
//     _oIdx.clear();

    return 0;
  }

//   if(_libHandle == 0)
//   {
//     _libHandle = dlopen(_fi.filePath().toLatin1().constData(), RTLD_NOW);
// 
//     if(_libHandle == 0)
//     {
//       fprintf(stderr, "Lv2Plugin::incReferences dlopen(%s) failed: %s\n",
//                     _fi.filePath().toLatin1().constData(), dlerror());
//       return 0;
//     }
// 
//     LADSPA_Descriptor_Function ladspadf = (LADSPA_Descriptor_Function)dlsym(_libHandle, "ladspa_descriptor");
//     if(ladspadf)
//     {
//       const LADSPA_Descriptor* descr;
//       for(unsigned long i = 0;; ++i)
//       {
//         descr = ladspadf(i);
//         if(descr == nullptr)
//           break;
// 
//         QString desc_label(descr->Label);
//         if(desc_label == label())
//         {
//           _plugin = descr;
//           break;
//         }
//       }
//     }
// 
//     if(_plugin != nullptr)
//     {
//       _name = QString(_plugin->Name);
//       _uniqueID = _plugin->UniqueID;
//       _maker = QString(_plugin->Maker);
//       _copyright = QString(_plugin->Copyright);
// 
//       _portCount = _plugin->PortCount;
// 
//       _inports = 0;
//       _outports = 0;
//       _controlInPorts = 0;
//       _controlOutPorts = 0;
//       
//       for(unsigned long k = 0; k < _portCount; ++k)
//       {
//         LADSPA_PortDescriptor pd = _plugin->PortDescriptors[k];
//         if(pd & LADSPA_PORT_AUDIO)
//         {
//           if(pd & LADSPA_PORT_INPUT)
//           {
//             ++_inports;
//             _iIdx.push_back(k);
//           }
//           else
//           if(pd & LADSPA_PORT_OUTPUT)
//           {
//             ++_outports;
//             _oIdx.push_back(k);
//           }
//         }
//         else
//         if(pd & LADSPA_PORT_CONTROL)
//         {
//           if(pd & LADSPA_PORT_INPUT)
//           {
//             ++_controlInPorts;
//             _pIdx.push_back(k);
//           }
//           else
//           if(pd & LADSPA_PORT_OUTPUT)
//           {
//             ++_controlOutPorts;
//             _poIdx.push_back(k);
//           }
//         }
//       }
//     }
//   }

//   if(_plugin == nullptr)
//   {
//     dlclose(_libHandle);
//     _libHandle = 0;
//     _references = 0;
//     fprintf(stderr, "Lv2Plugin::incReferences Error: %s no plugin!\n", _fi.filePath().toLatin1().constData());
//     return 0;
//   }

//   if ((_inports != _outports) || (LADSPA_IS_INPLACE_BROKEN(_plugin->Properties)))
//     _requiredFeatures |= NoInPlaceProcessing;
  
  _references = newref;

  return _references;
}



// SynthIF *LV2Synth::createSIF(SynthI *synthi)
// {
//    ++_instances;
//    LV2SynthIF *sif = new LV2SynthIF(synthi);
// 
//    if(!sif->init(this))
//    {
//       delete sif;
//       sif = nullptr;
//    }
// 
//    return sif;
// }

LV2_URID Lv2Plugin::mapUrid(const char *uri)
{
   return uridBiMap.map(uri);
}

const char *Lv2Plugin::unmapUrid(LV2_URID id)
{
   return uridBiMap.unmap(id);
}

// LV2SynthIF::~LV2SynthIF()
// {
//    if(_state != nullptr)
//    {
//       _state->deleteLater = true;
//       //_uiState->uiTimer->stopNextTime(false);
//       if(_state->pluginWindow != nullptr)
//          _state->pluginWindow->stopNextTime();
//       else
//          LV2Synth::lv2state_FreeState(_state);
//       _state = nullptr;
// 
//    }
// 
//    LV2_AUDIO_PORTS::iterator _itA = _audioInPorts.begin();
// 
//    for(; _itA != _audioInPorts.end(); ++_itA)
//    {
//       free((*_itA).buffer);
//    }
// 
//    _itA = _audioOutPorts.begin();
// 
//    for(; _itA != _audioOutPorts.end(); ++_itA)
//    {
//       free((*_itA).buffer);
//    }
// 
//    if(_audioInSilenceBuf)
//      free(_audioInSilenceBuf);
//   
//    if(_audioInBuffers)
//    {
//       delete [] _audioInBuffers;
//       _audioInBuffers = nullptr;
//    }
// 
//    if(_audioOutBuffers)
//    {
//       delete [] _audioOutBuffers;
//       _audioOutBuffers = nullptr;
//    }
// 
//    if(_controls)
//    {
//       delete [] _controls;
//    }
// 
//    if(_controlsOut)
//    {
//       delete [] _controlsOut;
//    }
// 
//    if(_ppifeatures)
//    {
//       delete [] _ppifeatures;
//       _ppifeatures = nullptr;
//    }
// 
//    if(_ifeatures)
//    {
//       delete [] _ifeatures;
//       _ifeatures = nullptr;
//    }
// }
// 
// LV2SynthIF::LV2SynthIF(SynthI *s): SynthIF(s)
// {
//    _synth = nullptr;
//    _handle = nullptr;
//    _audioInBuffers = nullptr;
//    _audioOutBuffers = nullptr;
//    _inports = 0;
//    _outports = 0;
//    _controls = nullptr;
//    _controlsOut = nullptr;
//    _inportsControl = 0;
//    _outportsControl = 0;
//    _inportsMidi = 0,
//    _outportsMidi = 0,
//    _audioInSilenceBuf = nullptr;
//    _ifeatures = nullptr;
//    _ppifeatures = nullptr;
//    _state = nullptr;
// }
// 
// bool LV2SynthIF::init(LV2Synth *s)
// {
//    _synth = s;
// 
//    //use LV2Synth features as template
// 
//    _state = new LV2PluginWrapper_State;
//    _state->inst = nullptr;
//    _state->widget = nullptr;
//    _state->uiInst = nullptr;
//    _state->plugInst = nullptr;
//    _state->_ifeatures = new LV2_Feature[SIZEOF_ARRAY(lv2Features)];
//    _state->_ppifeatures = new LV2_Feature *[SIZEOF_ARRAY(lv2Features) + 1];
//    _state->sif = this;
//    _state->synth = _synth;
// 
//    LV2Synth::lv2state_FillFeatures(_state);
// 
//    _state->handle = _handle = lilv_plugin_instantiate(_synth->_handle, (double)MusEGlobal::sampleRate, _state->_ppifeatures);
// 
//    if(_handle == nullptr)
//    {
//       delete [] _state->_ppifeatures;
//       _state->_ppifeatures = nullptr;
//       delete [] _state->_ifeatures;
//       _state->_ifeatures = nullptr;
//       return false;
//    }
// 
//    _audioInPorts = s->_audioInPorts;
//    _audioOutPorts = s->_audioOutPorts;
//    _controlInPorts = s->_controlInPorts;
//    _controlOutPorts = s->_controlOutPorts;
//    _inportsMidi = _state->midiInPorts.size();
//    _outportsMidi = _state->midiOutPorts.size();
// 
// 
// 
//    _inportsControl = _controlInPorts.size();
//    _outportsControl = _controlOutPorts.size();
// 
//    if(_inportsControl != 0)
//    {
//       _controls = new Port[_inportsControl];
//    }
//    else
//    {
//       _controls = nullptr;
//    }
// 
//    if(_outportsControl != 0)
//    {
//       _controlsOut = new Port[_outportsControl];
//    }
//    else
//    {
//       _controlsOut = nullptr;
//    }
// 
//    _synth->midiCtl2PortMap.clear();
//    _synth->port2MidiCtlMap.clear();
// 
//    for(size_t i = 0; i < _inportsControl; i++)
//    {
//       uint32_t idx = _controlInPorts [i].index;
//       _controls [i].idx = idx;
//       _controls [i].val = _controls [i].tmpVal = _controlInPorts [i].defVal = _synth->_pluginControlsDefault [idx];
//       if(_synth->_hasFreeWheelPort && _synth->_freeWheelPortIndex == i)
//          _controls [i].enCtrl = false;
//       else
//          _controls [i].enCtrl = true;
//       _controlInPorts [i].minVal = _synth->_pluginControlsMin [idx];
//       _controlInPorts [i].maxVal = _synth->_pluginControlsMax [idx];
// 
//       int ctlnum = CTRL_NRPN14_OFFSET + 0x2000 + i;
// 
//       // We have a controller number! Insert it and the lv2 port number into both maps.
//       _synth->midiCtl2PortMap.insert(std::pair<int, int>(ctlnum, i));
//       _synth->port2MidiCtlMap.insert(std::pair<int, int>(i, ctlnum));
// 
//       int id = genACnum(MAX_PLUGINS, i);
//       CtrlList *cl;
//       CtrlListList *cll = track()->controller();
//       iCtrlList icl = cll->find(id);
// 
//       if(icl == cll->end())
//       {
//          cl = new CtrlList(id);
//          cll->add(cl);
//          cl->setCurVal(_controls[i].val);
//       }
//       else
//       {
//          cl = icl->second;
//          _controls[i].val = cl->curVal();
//       }
// 
//       cl->setRange(_synth->_pluginControlsMin [idx], _synth->_pluginControlsMax [idx]);
//       cl->setName(QString(_controlInPorts [i].cName));
//       CtrlValueType vt = VAL_LINEAR;
//       switch(_controlInPorts [i].cType)
//       {
//       case LV2_PORT_CONTINUOUS:
//          vt = VAL_LINEAR;
//          break;
//       case LV2_PORT_DISCRETE:
//       case LV2_PORT_INTEGER:
//          vt = VAL_INT;
//          break;
//       case LV2_PORT_LOGARITHMIC:
//          vt = VAL_LOG;
//          break;
//       case LV2_PORT_TRIGGER:
//          vt = VAL_BOOL;
//          break;
//       default:
//          break;
//       }
// 
//       cl->setValueType(vt);
//       cl->setMode(((_controlInPorts [i].cType == LV2_PORT_CONTINUOUS)
//                    ||(_controlInPorts [i].cType == LV2_PORT_LOGARITHMIC))? CtrlList::INTERPOLATE : CtrlList::DISCRETE);
// 
//       if(!_controlInPorts [i].isCVPort)
//          lilv_instance_connect_port(_handle, idx, &_controls [i].val);
//    }
// 
//    for(size_t i = 0; i < _outportsControl; i++)
//    {
//       uint32_t idx = _controlOutPorts [i].index;
//       _controlsOut[i].idx = idx;
//       _controlsOut[i].val    = 0.0;
//       _controlsOut[i].tmpVal = 0.0;
//       _controlsOut[i].enCtrl  = false;
//       _controlOutPorts [i].defVal = _controlOutPorts [i].minVal = _controlOutPorts [i].maxVal = 0.0;
//       if(!_controlOutPorts [i].isCVPort)
//          lilv_instance_connect_port(_handle, idx, &_controlsOut[i].val);
//    }
// 
// 
//    int rv = posix_memalign((void **)&_audioInSilenceBuf, 16, sizeof(float) * MusEGlobal::segmentSize);
// 
//    if(rv != 0)
//    {
//       fprintf(stderr, "ERROR: LV2SynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
//       abort();
//    }
// 
//    if(MusEGlobal::config.useDenormalBias)
//    {
//       for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
//       {
//          _audioInSilenceBuf[q] = MusEGlobal::denormalBias;
//       }
//    }
//    else
//    {
//       memset(_audioInSilenceBuf, 0, sizeof(float) * MusEGlobal::segmentSize);
//    }
// 
//    //cache number of ports
//    _inports = _audioInPorts.size();
//    _outports = _audioOutPorts.size();
// 
// 
//    if(_inports > 0)
//    {
//       _audioInBuffers = new float*[_inports];
// 
//       for(size_t i = 0; i < _inports; i++)
//       {
//          int rv = posix_memalign((void **)&_audioInBuffers [i], 16, sizeof(float) * MusEGlobal::segmentSize);
// 
//          if(rv != 0)
//          {
//             fprintf(stderr, "ERROR: LV2SynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
//             abort();
//          }
// 
//          if(MusEGlobal::config.useDenormalBias)
//          {
//             for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
//             {
//                _audioInBuffers [i][q] = MusEGlobal::denormalBias;
//             }
//          }
//          else
//          {
//             memset(_audioInBuffers [i], 0, sizeof(float) * MusEGlobal::segmentSize);
//          }
//          
//          _audioInPorts [i].buffer = _audioInBuffers [i];
//          lilv_instance_connect_port(_handle, _audioInPorts [i].index, _audioInBuffers [i]);
//       }
//    }
// 
//    if(_outports > 0)
//    {
//       _audioOutBuffers = new float*[_outports];
// 
//       for(size_t i = 0; i < _outports; i++)
//       {
//          int rv = posix_memalign((void **)&_audioOutBuffers [i], 16, sizeof(float) * MusEGlobal::segmentSize);
// 
//          if(rv != 0)
//          {
//             fprintf(stderr, "ERROR: LV2SynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
//             abort();
//          }
// 
//          if(MusEGlobal::config.useDenormalBias)
//          {
//             for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
//             {
//                _audioOutBuffers [i][q] = MusEGlobal::denormalBias;
//             }
//          }
//          else
//          {
//             memset(_audioOutBuffers [i], 0, sizeof(float) * MusEGlobal::segmentSize);
//          }
// 
//          _audioOutPorts [i].buffer = _audioOutBuffers [i];
//          lilv_instance_connect_port(_handle, _audioOutPorts [i].index, _audioOutBuffers [i]);
//       }
//    }
// 
//    LV2Synth::lv2state_PostInstantiate(_state);
//    activate();
// 
// 
// 
//    return true;
// 
// }
// 
// void LV2SynthIF::doSelectProgram(unsigned char channel, int bankH, int bankL, int prog)
// {
// //    // Only if there's something to change...
// //    if(bankH >= 128 && bankL >= 128 && prog >= 128)
// //      return;
//   
//    if(bankH > 127) // Map "dont care" to 0
//      bankH = 0;
//    if(bankL > 127)
//      bankL = 0;
//    if(prog > 127)
//      prog = 0;
//     
//    const int bank = (bankH << 8) | bankL;
//   
//    if(_state && _state->prgIface && (_state->prgIface->select_program || _state->prgIface->select_program_for_channel))
//    {
//       if(_state->newPrgIface)
//          _state->prgIface->select_program_for_channel(lilv_instance_get_handle(_state->handle), channel, (uint32_t)bank, (uint32_t)prog);
//       else
//          _state->prgIface->select_program(lilv_instance_get_handle(_state->handle), (uint32_t)bank, (uint32_t)prog);
// 
// 
// 
//       /*
//       * A plugin is permitted to re-write the values of its input
//       * control ports when select_program is called. The host should
//       * re-read the input control port values and update its own
//       * records appropriately. (This is the only circumstance in which
//       * a LV2 plugin is allowed to modify its own control-input ports.)
//       */
//       if(id() != -1)
//       {
//          for(unsigned long k = 0; k < _inportsControl; ++k)
//          {
//             // We're in the audio thread context: no need to send a message, just modify directly.
//             synti->setPluginCtrlVal(genACnum(id(), k), _controls[k].val);
//          }
//       }
// 
//       //set update ui program flag
//       _state->uiChannel = channel;
//       _state->uiBank = bank;
//       _state->uiProg = prog;
//       _state->uiDoSelectPrg = true;
//    }
// }
// 
// void LV2SynthIF::sendLv2MidiEvent(LV2EvBuf *evBuf, long frame, int paramCount, uint8_t a, uint8_t b, uint8_t c)
// {
//   if(paramCount < 1 || paramCount > 3)
//     return;
//   
//    if(evBuf)
//    {
//       uint8_t midiEv [paramCount];
//       midiEv [0] = a;
//       if(paramCount >= 2)
//         midiEv [1] = b;
//       if(paramCount == 3)
//         midiEv [2] = c;
// #ifdef LV2_EVENT_BUFFER_SUPPORT
//       evBuf->write(frame, 0, _synth->_midi_event_id, paramCount, midiEv);
// #else
//       evBuf->write(frame, _synth->_midi_event_id, paramCount, midiEv);
// #endif
//    }
// }
// 
// int LV2SynthIF::channels() const
// {
//    return (_outports) > MAX_CHANNELS ? MAX_CHANNELS : (_outports) ;
// 
// }
// 
// int LV2SynthIF::totalInChannels() const
// {
//    return _inports;
// }
// 
// int LV2SynthIF::totalOutChannels() const
// {
//    return _outports;
// }
// 
// void LV2SynthIF::activate()
// {
//    if(_handle)
//    {
//       lilv_instance_activate(_handle);
//    }
// }
// 
// 
// void LV2SynthIF::deactivate()
// {
//    if(_handle)
//    {
//       lilv_instance_deactivate(_handle);
//    }
// 
// }
// 
// void LV2SynthIF::deactivate3()
// {
//    deactivate();
// }
// 
// int LV2SynthIF::eventsPending() const
// {
//    //TODO: what's this?
//    return 0;
// }
// 
// bool LV2SynthIF::lv2MidiControlValues(size_t port, int ctlnum, int *min, int *max, int *def)
// {
// 
//    float fmin, fmax, fdef;
//    int   imin;
//    float frng;
// 
//    fdef = _controlInPorts [port].defVal;
//    fmin = _controlInPorts [port].minVal;
//    fmax = _controlInPorts [port].maxVal;
//    bool hasdef = (fdef == fdef);
// 
//    if(fmin != fmin)
//    {
//       fmin = 0.0;
//    }
// 
//    if(fmax != fmax)
//    {
//       fmax = 0.0;
//    }
// 
//    MidiController::ControllerType t = midiControllerType(ctlnum);
// 
// #ifdef PLUGIN_DEBUGIN
//    printf("lv2MidiControlValues: ctlnum:%d ladspa port:%lu has default?:%d default:%f\n", ctlnum, port, hasdef, fdef);
// #endif
// 
// 
//    frng = fmax - fmin;
//    imin = lrintf(fmin);
//    //imax = lrintf(fmax);
// 
//    int ctlmn = 0;
//    int ctlmx = 127;// Avoid divide-by-zero error below.
// 
// 
// #ifdef PLUGIN_DEBUGIN
//    printf("lv2MidiControlValues: port min:%f max:%f \n", fmin, fmax);
// #endif
// 
//    bool isneg = (imin < 0);
//    int bias = 0;
// 
//    switch(t)
//    {
//    case MidiController::RPN:
//    case MidiController::NRPN:
//    case MidiController::Controller7:
//       if(isneg)
//       {
//          ctlmn = -64;
//          ctlmx = 63;
//          bias = -64;
//       }
//       else
//       {
//          ctlmn = 0;
//          ctlmx = 127;
//       }
// 
//       break;
// 
//    case MidiController::Controller14:
//    case MidiController::RPN14:
//    case MidiController::NRPN14:
//       if(isneg)
//       {
//          ctlmn = -8192;
//          ctlmx = 8191;
//          bias = -8192;
//       }
//       else
//       {
//          ctlmn = 0;
//          ctlmx = 16383;
//       }
// 
//       break;
// 
//    case MidiController::Program:
//       ctlmn = 0;
//       ctlmx = 0x3fff;     // FIXME: Really should not happen or be allowed. What to do here...
//       break;
// 
//    case MidiController::Pitch:
//       ctlmn = -8192;
//       ctlmx = 8191;
//       break;
// 
//    case MidiController::Velo:        // cannot happen
//    default:
//       break;
//    }
// 
//    float fctlrng = float(ctlmx - ctlmn);
// 
// 
//    // It's a floating point control, just use wide open maximum range.
//    *min = ctlmn;
//    *max = ctlmx;
// 
//    float normdef = (frng == 0.0) ? 0.0 : fdef / frng;
//    fdef = normdef * fctlrng;
// 
//    // FIXME: TODO: Incorrect... Fix this somewhat more trivial stuff later....
// 
//    *def = (int)lrintf(fdef) + bias;
// 
// #ifdef PLUGIN_DEBUGIN
//    printf("lv2MidiControlValues: setting default:%d\n", *def);
// #endif
// 
//    return hasdef;
// }
// 
// //---------------------------------------------------------
// //   midi2LadspaValue
// //---------------------------------------------------------
// 
// float LV2SynthIF::midi2Lv2Value(unsigned long port, int ctlnum, int val)
// {
// 
//    float fmin, fmax;
//    int   imin;
//    float frng;
// 
//    MidiController::ControllerType t = midiControllerType(ctlnum);
// 
// #ifdef PLUGIN_DEBUGIN
//    printf("midi2Lv2Value: ctlnum:%d port:%lu val:%d\n", ctlnum, port, val);
// #endif
// 
//    fmin = _controlInPorts [port].minVal;
//    fmax = _controlInPorts [port].maxVal;
// 
//    if(fmin != fmin)
//    {
//       fmin = 0.0;
//    }
// 
//    if(fmax != fmax)
//    {
//       fmax = 0.0;
//    }
// 
// 
// 
//    frng = fmax - fmin;
//    imin = lrintf(fmin);
// 
// 
//    int ctlmn = 0;
//    int ctlmx = 127;
// 
// #ifdef PLUGIN_DEBUGIN
//    printf("midi2Lv2Value: port min:%f max:%f \n", fmin, fmax);
// #endif
// 
//    bool isneg = (imin < 0);
//    int bval = val;
//    //int cval = val;
// 
//    switch(t)
//    {
//    case MidiController::RPN:
//    case MidiController::NRPN:
//    case MidiController::Controller7:
//       if(isneg)
//       {
//          ctlmn = -64;
//          ctlmx = 63;
//          bval -= 64;
//          //cval -= 64;
//       }
//       else
//       {
//          ctlmn = 0;
//          ctlmx = 127;
//         //cval -= 64;
//       }
// 
//       break;
// 
//    case MidiController::Controller14:
//    case MidiController::RPN14:
//    case MidiController::NRPN14:
//       if(isneg)
//       {
//          ctlmn = -8192;
//          ctlmx = 8191;
//          bval -= 8192;
//          //cval -= 8192;
//       }
//       else
//       {
//          ctlmn = 0;
//          ctlmx = 16383;
//          //cval -= 8192;
//       }
// 
//       break;
// 
//    case MidiController::Program:
//       ctlmn = 0;
//       ctlmx = 0xffffff;
//       break;
// 
//    case MidiController::Pitch:
//       ctlmn = -8192;
//       ctlmx = 8191;
//       break;
// 
//    case MidiController::Velo:        // cannot happen
//    default:
//       break;
//    }
// 
//    int ctlrng = ctlmx - ctlmn;
//    float fctlrng = float(ctlmx - ctlmn);
// 
// 
//    // Avoid divide-by-zero error below.
//    if(ctlrng == 0)
//    {
//       return 0.0;
//    }
// 
//    // It's a floating point control, just use wide open maximum range.
//    float normval = float(bval) / fctlrng;
//    float ret = normval * frng + fmin;
// 
// #ifdef PLUGIN_DEBUGIN
//    printf("midi2Lv2Value: float returning:%f\n", ret);
// #endif
// 
//    return ret;
// }
// 
// int LV2SynthIF::getControllerInfo(int id, QString* name, int *ctrl, int *min, int *max, int *initval)
// {
//    size_t _id = (size_t)id;
// 
//    if(_id == _inportsControl || _id == _inportsControl + 1)
//    {
//       //
//       // It is unknown at this point whether or not a synth recognizes aftertouch and poly aftertouch
//       //  (channel and key pressure) midi messages, so add support for them now (as controllers).
//       //
//       if(_id == _inportsControl)
//       {
//          *ctrl = CTRL_POLYAFTER;
//       }
//       else if(_id == _inportsControl + 1)
//       {
//          *ctrl = CTRL_AFTERTOUCH;
//       }
// 
//       *min  = 0;
//       *max  = 127;
//       *initval = CTRL_VAL_UNKNOWN;
//       *name = midiCtrlName(*ctrl);
//       return ++_id;
//    }
//    else if(_id >= _inportsControl + 2)
//    {
//       return 0;
//    }
// 
//    int ctlnum; // = DSSI_NONE;
// 
//    // No controller number? Give it one.
//    //if(ctlnum == DSSI_NONE)
//    //{
//       // Simple but flawed solution: Start them at 0x60000 + 0x2000 = 0x62000. Max NRPN number is 0x3fff.
//       ctlnum = CTRL_NRPN14_OFFSET + 0x2000 + _id;
//    //}
// 
// 
//    int def = CTRL_VAL_UNKNOWN;
// 
//    if(lv2MidiControlValues(_id, ctlnum, min, max, &def))
//    {
//       *initval = def;
//    }
//    else
//    {
//       *initval = CTRL_VAL_UNKNOWN;
//    }
// 
// #ifdef DEBUG_LV2
//    printf("LV2SynthIF::getControllerInfo passed ctlnum:%d min:%d max:%d initval:%d\n", ctlnum, *min, *max, *initval);
// #endif
// 
//    *ctrl = ctlnum;
//    *name = QString(_controlInPorts[_id].cName);
//    return ++_id;
// 
// }
// 
// 
// 
// bool LV2SynthIF::processEvent(const MidiPlayEvent &e, LV2EvBuf *evBuf, long frame)
// {
//    int chn = e.channel();
//    int a   = e.dataA();
//    int b   = e.dataB();
//    int type = e.type();
// 
// #ifdef LV2_DEBUG
//    fprintf(stderr, "LV2SynthIF::processEvent midi event type:%d chn:%d a:%d b:%d\n", e.type(), chn, a, b);
// #endif
// 
//    // REMOVE Tim. Noteoff. Added.
//    const MidiInstrument::NoteOffMode nom = synti->noteOffMode();
//   
//    switch(type)
//    {
//    case ME_NOTEON:
// #ifdef LV2_DEBUG
//       fprintf(stderr, "LV2SynthIF::processEvent midi event is ME_NOTEON\n");
// #endif
//       // REMOVE Tim. Noteoff. Changed.
// //       if(b)
// //       {
// //          sendLv2MidiEvent(evBuf, frame, (type | chn) & 0xff, a & 0x7f, b & 0x7f);
// //       }
// //       else
// //       {
// //          sendLv2MidiEvent(evBuf, frame, (ME_NOTEOFF | chn) & 0xff, a & 0x7f, 0);
// //       }
//       if(b == 0)
//       {
//         // Handle zero-velocity note ons. Technically this is an error because internal midi paths
//         //  are now all 'note-off' without zero-vel note ons - they're converted to note offs.
//         // Nothing should be setting a Note type Event's on velocity to zero.
//         // But just in case... If we get this warning, it means there is still code to change.
//         fprintf(stderr, "LV2SynthIF::processEvent: Warning: Zero-vel note on: time:%d type:%d (ME_NOTEON) ch:%d A:%d B:%d\n", e.time(), e.type(), chn, a, b);  
//         switch(nom)
//         {
//           // Instrument uses note offs. Convert to zero-vel note off.
//           case MidiInstrument::NoteOffAll:
//             //if(MusEGlobal::midiOutputTrace)
//             //  fprintf(stderr, "MidiOut: LV2: Following event will be converted to zero-velocity note off:\n");
//             sendLv2MidiEvent(evBuf, frame, 3, (ME_NOTEOFF | chn) & 0xff, a & 0x7f, 0);
//           break;
//           
//           // Instrument uses no note offs at all. Send as-is.
//           case MidiInstrument::NoteOffNone:
//           // Instrument converts all note offs to zero-vel note ons. Send as-is.
//           case MidiInstrument::NoteOffConvertToZVNoteOn:
//             sendLv2MidiEvent(evBuf, frame, 3, (type | chn) & 0xff, a & 0x7f, b & 0x7f);
//           break;
//         }
//       }
//       else
//         sendLv2MidiEvent(evBuf, frame, 3, (type | chn) & 0xff, a & 0x7f, b & 0x7f);
//       
//       break;
// 
//    case ME_NOTEOFF:
// #ifdef LV2_DEBUG
//       fprintf(stderr, "LV2SynthIF::processEvent midi event is ME_NOTEOFF\n");
// #endif
// 
//       // REMOVE Tim. Noteoff. Changed.
// //       sendLv2MidiEvent(evBuf, frame, (type | chn) & 0xff, a & 0x7f, b & 0x7f);
//       switch(nom)
//       {
//         // Instrument uses note offs. Send as-is.
//         case MidiInstrument::NoteOffAll:
//           sendLv2MidiEvent(evBuf, frame, 3, (type | chn) & 0xff, a & 0x7f, b & 0x7f);
//         break;
//         
//         // Instrument uses no note offs at all. Send nothing. Eat up the event - return false.
//         case MidiInstrument::NoteOffNone:
//           return false;
//           
//         // Instrument converts all note offs to zero-vel note ons. Convert to zero-vel note on.
//         case MidiInstrument::NoteOffConvertToZVNoteOn:
//           //if(MusEGlobal::midiOutputTrace)
//           //  fprintf(stderr, "MidiOut: LV2: Following event will be converted to zero-velocity note on:\n");
//           sendLv2MidiEvent(evBuf, frame, 3, (ME_NOTEON | chn) & 0xff, a & 0x7f, 0);
//         break;
//       }
//       
//       break;
// 
//    case ME_PROGRAM:
//    {
// #ifdef LV2_DEBUG
//       fprintf(stderr, "LV2SynthIF::processEvent midi event is ME_PROGRAM\n");
// #endif
// 
//       int hb, lb;
//       synti->currentProg(chn, nullptr, &lb, &hb);
//       synti->setCurrentProg(chn, a & 0xff, lb, hb);
//       doSelectProgram(chn, hb, lb, a); 
// 
//       // Event pointer not filled. Return false.
//       return false;
//    }
//    break;
// 
//    case ME_CONTROLLER:
//    {
// #ifdef LV2_DEBUG
//       fprintf(stderr, "LV2SynthIF::processEvent midi event is ME_CONTROLLER\n");
// #endif
// 
//       // Our internal hwCtrl controllers support the 'unknown' value.
//       // Don't send 'unknown' values to the driver. Ignore and return no error.
//       if(b == CTRL_VAL_UNKNOWN)
//         return false;
//             
//       if(a == CTRL_PROGRAM)
//       {
// #ifdef LV2_DEBUG
//          fprintf(stderr, "LV2SynthIF::processEvent midi event is ME_CONTROLLER, dataA is CTRL_PROGRAM\n");
// #endif
// 
//         int hb = (b >> 16) & 0xff;
//         int lb = (b >> 8) & 0xff;
//         int pr = b & 0xff;
//         synti->setCurrentProg(chn, pr, lb, hb);
//         doSelectProgram(chn, hb, lb, pr);
// 
//          // Event pointer not filled. Return false.
//          return false;
//       }
// 
//       if(a == CTRL_HBANK)
//       {
//         int lb, pr;
//         synti->currentProg(chn, &pr, &lb, nullptr);
//         synti->setCurrentProg(chn, pr, lb, b & 0xff);
//         doSelectProgram(chn, b, lb, pr);
//         // Event pointer not filled. Return false.
//         return false;
//       }
//       
//       if(a == CTRL_LBANK)
//       {
//         int hb, pr;
//         synti->currentProg(chn, &pr, nullptr, &hb);
//         synti->setCurrentProg(chn, pr, b & 0xff, hb);
//         doSelectProgram(chn, hb, b, pr);
//         // Event pointer not filled. Return false.
//         return false;
//       }
//       
//       if(a == CTRL_PITCH)
//       {
// #ifdef LV2_DEBUG
//          fprintf(stderr, "LV2SynthIF::processEvent midi event is ME_CONTROLLER, dataA is CTRL_PITCH\n");
// #endif
// 
//          b += 8192;
//          sendLv2MidiEvent(evBuf, frame, 3, (ME_PITCHBEND | chn) & 0xff, b & 0x7f, (b >> 7) & 0x7f);
//          // Event pointer filled. Return true.
//          return true;
//       }
// 
//       if(a == CTRL_AFTERTOUCH)
//       {
// #ifdef LV2_DEBUG
//          fprintf(stderr, "LV2SynthIF::processEvent midi event is ME_CONTROLLER, dataA is CTRL_AFTERTOUCH\n");
// #endif
//          sendLv2MidiEvent(evBuf, frame, 2, (ME_AFTERTOUCH | chn) & 0xff, b & 0x7f, 0);
//          // Event pointer filled. Return true.
//          return true;
//       }
// 
//       if((a | 0xff)  == CTRL_POLYAFTER)
//       {
// #ifdef LV2_DEBUG
//          fprintf(stderr, "LV2SynthIF::processEvent midi event is ME_CONTROLLER, dataA is CTRL_POLYAFTER\n");
// #endif
//          sendLv2MidiEvent(evBuf, frame, 3, (ME_POLYAFTER | chn) & 0xff, a & 0x7f, b & 0x7f);
//          // Event pointer filled. Return true.
//          return true;
//       }
// 
//       ciMidiCtl2LadspaPort ip = _synth->midiCtl2PortMap.find(a);
// 
//       // Is it just a regular midi controller, not mapped to a LADSPA port (either by the plugin or by us)?
//       // NOTE: There's no way to tell which of these controllers is supported by the plugin.
//       // For example sustain footpedal or pitch bend may be supported, but not mapped to any LADSPA port.
//       if(ip == _synth->midiCtl2PortMap.end())
//       {
// 
//          if(midiControllerType(a) == MidiController::NRPN14
//             || midiControllerType(a) == MidiController::NRPN)
//          {
//             //send full nrpn control sequence (4 values):
//             // 99 + ((a & 0xff00) >> 8) - first byte
//             // 98 + (a & 0xff) - second byte
//             // 6 + ((b & 0x3f80) >> 7) - third byte
//             // 38 + (b & 0x7f) - fourth byte
// 
// 
//             sendLv2MidiEvent(evBuf, frame, 3, (type | chn) & 0xff, 99, ((a & 0xff00) >> 8));
//             sendLv2MidiEvent(evBuf, frame, 3, (type | chn) & 0xff, 98, (a & 0xff));
//             sendLv2MidiEvent(evBuf, frame, 3, (type | chn) & 0xff, 6, ((b & 0x3f80) >> 7));
//             sendLv2MidiEvent(evBuf, frame, 3, (type | chn) & 0xff, 38, (b & 0x7f));
//             return true;
// 
//          }
//          else if(midiControllerType(a) != MidiController::Controller7)
//          {
//             return false;   // Event pointer not filled. Return false.
//          }
// 
//          // Fill the event.
// #ifdef LV2_DEBUG
//          printf("LV2SynthIF::processEvent non-ladspa filling midi event chn:%d dataA:%d dataB:%d\n", chn, a, b);
// #endif
//          sendLv2MidiEvent(evBuf, frame, 3, (type | chn) & 0xff, a & 0x7f, b & 0x7f);
//          return true;
//       }
// 
//       unsigned long k = ip->second;
//       int ctlnum; // = DSSI_NONE;
// 
//       // No midi controller for the ladspa port? Send to ladspa control.
//       //if(ctlnum == DSSI_NONE)
//       //{
//          // Sanity check.
//          if(k > _inportsControl)
//          {
//             return false;
//          }
// 
//          // Simple but flawed solution: Start them at 0x60000 + 0x2000 = 0x62000. Max NRPN number is 0x3fff.
//          ctlnum = k + (CTRL_NRPN14_OFFSET + 0x2000);
//       //}
// 
// 
//       float val = midi2Lv2Value(k, ctlnum, b);
// 
// #ifdef LV2_DEBUG
//       //fprintf(stderr, "LV2SynthIF::processEvent control port:%lu port:%lu dataA:%d Converting val from:%d to lv2:%f\n", i, k, a, b, val);
//       fprintf(stderr, "LV2SynthIF::processEvent port:%lu dataA:%d Converting val from:%d to lv2:%f\n", k, a, b, val);
// #endif
// 
//       // Set the lv2 port value.
//       _controls[k].val = val;
// 
//       // Need to update the automation value, otherwise it overwrites later with the last automation value.
//       if(id() != -1)
//          // We're in the audio thread context: no need to send a message, just modify directly.
//       {
//          synti->setPluginCtrlVal(genACnum(id(), k), val);
//       }
// 
//       // Since we absorbed the message as a lv2 control change, return false - the event is not filled.
//       return false;
//    }
//    break;
// 
//    case ME_PITCHBEND:
//       a += 8192;
//       sendLv2MidiEvent(evBuf, frame, 3, (type | chn) & 0xff, a & 0x7f, (a >> 7) & 0x7f);
//       break;
// 
//    case ME_AFTERTOUCH:
//       sendLv2MidiEvent(evBuf, frame, 2, (type | chn) & 0xff, a & 0x7f, 0);
//       break;
// 
//    case ME_POLYAFTER:
//       sendLv2MidiEvent(evBuf, frame, 3, (type | chn) & 0xff, a & 0x7f, b & 0x7f);
//       break;
// 
//    default:
//       if(MusEGlobal::debugMsg)
//       {
//          fprintf(stderr, "LV2SynthIF::processEvent midi event unknown type:%d\n", e.type());
//       }
// 
//       // Event not filled.
//       return false;
//       break;
//    }
// 
//    return true;
// 
// }
// 
// 
// bool LV2SynthIF::getData(MidiPort *, unsigned int pos, int ports, unsigned int nframes, float **buffer)
// {
//    const unsigned int syncFrame = MusEGlobal::audio->curSyncFrame();
//    // All ports must be connected to something!
//    const unsigned long nop = ((unsigned long) ports) > _outports ? _outports : ((unsigned long) ports);
//    const bool usefixedrate = (requiredFeatures() & Plugin::FixedBlockSize);;
//    const unsigned long min_per = (usefixedrate || MusEGlobal::config.minControlProcessPeriod > nframes) ? nframes : MusEGlobal::config.minControlProcessPeriod;
//    const unsigned long min_per_mask = min_per - 1; // min_per must be power of 2
// 
//    unsigned long sample = 0;
//    AudioTrack *atrack = track();
//    const AutomationType at = atrack->automationType();
//    const bool no_auto = !MusEGlobal::automation || at == AUTO_OFF;
//    CtrlListList *cll = atrack->controller();
//    ciCtrlList icl_first;
//    const int plug_id = id();
// 
//    LV2EvBuf *evBuf = (_inportsMidi > 0) ? _state->midiInPorts [0].buffer : NULL;
// 
//    //set freewheeling property if plugin supports it
//    if(_synth->_hasFreeWheelPort)
//    {
//       _controls [_synth->_freeWheelPortIndex].val = MusEGlobal::audio->freewheel() ? 1.0f : 0.0f;
//    }
// 
//    if(plug_id != -1 && ports != 0)  // Don't bother if not 'running'.
//    {
//       icl_first = cll->lower_bound(genACnum(plug_id, 0));
//    }
// 
//    bool used_in_chan_array[_inports]; // Don't bother initializing if not 'running'. 
//   
//    // Don't bother if not 'running'.
//    if(ports != 0)
//    {      
//       // Initialize the array.
//       for(size_t i = 0; i < _inports; ++i)
//         used_in_chan_array[i] = false;
//       
//       if(!atrack->noInRoute())
//       {
//          RouteList *irl = atrack->inRoutes();
//          for(ciRoute i = irl->begin(); i != irl->end(); ++i)
//          {
//             if(i->track->isMidiTrack())
//               continue;
//             // Only this synth knows how many destination channels there are, 
//             //  while only the track knows how many source channels there are.
//             // So take care of the destination channels here, and let the track handle the source channels.
//             const int dst_ch = i->channel <= -1 ? 0 : i->channel;
//             if((unsigned long)dst_ch >= _inports)
//               continue;
//             const int dst_chs = i->channels <= -1 ? _inports : i->channels;
//             //const int total_ins = atrack->totalRoutableInputs(Route::TRACK_ROUTE);
//             const int src_ch = i->remoteChannel <= -1 ? 0 : i->remoteChannel;
//             const int src_chs = i->channels;
// 
//             int fin_dst_chs = dst_chs;
//             if((unsigned long)(dst_ch + fin_dst_chs) > _inports)
//               fin_dst_chs = _inports - dst_ch;
//                 
//             static_cast<AudioTrack*>(i->track)->copyData(pos, 
//                                                         dst_ch, dst_chs, fin_dst_chs, 
//                                                         src_ch, src_chs, 
//                                                         nframes, &_audioInBuffers[0], 
//                                                         false, used_in_chan_array);
//             const int nxt_ch = dst_ch + fin_dst_chs;
//             for(int ch = dst_ch; ch < nxt_ch; ++ch)
//               used_in_chan_array[ch] = true;
//          }
//       }
//    }
// 
// 
//    int cur_slice = 0;
// 
//    while(sample < nframes)
//    {
//       unsigned long nsamp = nframes - sample;
//       const unsigned long slice_frame = pos + sample;
// 
//       //
//       // Process automation control values, while also determining the maximum acceptable
//       //  size of this run. Further processing, from FIFOs for example, can lower the size
//       //  from there, but this section determines where the next highest maximum frame
//       //  absolutely needs to be for smooth playback of the controller value stream...
//       //
//       if(ports != 0)    // Don't bother if not 'running'.
//       {
//          ciCtrlList icl = icl_first;
// 
//          for(unsigned long k = 0; k < _inportsControl; ++k)
//          {            
//             //don't process freewheel port
//             if(_synth->_hasFreeWheelPort && _synth->_freeWheelPortIndex == k)
//                continue;
// 
//             CtrlList *cl = (cll && plug_id != -1 && icl != cll->end()) ? icl->second : NULL;
//             CtrlInterpolate &ci = _controls[k].interp;
// 
//             // Always refresh the interpolate struct at first, since things may have changed.
//             // Or if the frame is outside of the interpolate range - and eStop is not true.  // FIXME TODO: Be sure these comparisons are correct.
//             if(cur_slice == 0 || (!ci.eStop && MusEGlobal::audio->isPlaying() &&
//                                   (slice_frame < (unsigned long)ci.sFrame || (ci.eFrame != -1 && slice_frame >= (unsigned long)ci.eFrame))))
//             {
//                if(cl && plug_id != -1 && (unsigned long)cl->id() == genACnum(plug_id, k))
//                {
//                   cl->getInterpolation(slice_frame, no_auto || !_controls[k].enCtrl, &ci);
// 
//                   if(icl != cll->end())
//                   {
//                      ++icl;
//                   }
//                }
//                else
//                {
//                   // No matching controller, or end. Just copy the current value into the interpolator.
//                   // Keep the current icl iterator, because since they are sorted by frames,
//                   //  if the IDs didn't match it means we can just let k catch up with icl.
//                   ci.sFrame   = 0;
//                   ci.eFrame   = -1;
//                   ci.sVal     = _controls[k].val;
//                   ci.eVal     = ci.sVal;
//                   ci.doInterp = false;
//                   ci.eStop    = false;
//                }
//             }
//             else
//             {
//                if(ci.eStop && ci.eFrame != -1 && slice_frame >= (unsigned long)ci.eFrame)  // FIXME TODO: Get that comparison right.
//                {
//                   // Clear the stop condition and set up the interp struct appropriately as an endless value.
//                   ci.sFrame   = 0; //ci->eFrame;
//                   ci.eFrame   = -1;
//                   ci.sVal     = ci.eVal;
//                   ci.doInterp = false;
//                   ci.eStop    = false;
//                }
// 
//                if(cl && cll && icl != cll->end())
//                {
//                   ++icl;
//                }
//             }
// 
//             if(!usefixedrate && MusEGlobal::audio->isPlaying())
//             {
//                unsigned long samps = nsamp;
// 
//                if(ci.eFrame != -1)
//                {
//                   samps = (unsigned long)ci.eFrame - slice_frame;
//                }
// 
//                if(!ci.doInterp && samps > min_per)
//                {
//                   samps &= ~min_per_mask;
// 
//                   if((samps & min_per_mask) != 0)
//                   {
//                      samps += min_per;
//                   }
//                }
//                else
//                {
//                   samps = min_per;
//                }
// 
//                if(samps < nsamp)
//                {
//                   nsamp = samps;
//                }
// 
//             }
// 
//             if(ci.doInterp && cl)
//             {
//                _controls[k].val = cl->interpolate(MusEGlobal::audio->isPlaying() ? slice_frame : pos, ci);
//             }
//             else
//             {
//                _controls[k].val = ci.sVal;
//             }
//             _state->controlsMask [k] = true;
// 
// #ifdef LV2_DEBUG_PROCESS
//             fprintf(stderr, "LV2SynthIF::getData k:%lu val:%f sample:%lu ci.eFrame:%d nsamp:%lu \n", k, _controls[k].val, sample, ci.eFrame, nsamp);
// #endif
// 
//          }
//       }
// 
// 
//       bool found = false;
//       unsigned long frame = 0;
//       unsigned long index = 0;
// 
//       // Get all control ring buffer items valid for this time period...
//       while(!_controlFifo.isEmpty())
//       {
//          unsigned long evframe;
//          const ControlEvent& v = _controlFifo.peek();
//          // The events happened in the last period or even before that. Shift into this period with + n. This will sync with audio.
//          // If the events happened even before current frame - n, make sure they are counted immediately as zero-frame.
//          evframe = (syncFrame > v.frame + nframes) ? 0 : v.frame - syncFrame + nframes;
// 
// #ifdef DEBUG_LV2
//          fprintf(stderr, "LV2SynthIF::getData found:%d evframe:%lu frame:%lu  event frame:%lu idx:%lu val:%f unique:%d\n",
//                  found, evframe, frame, v.frame, v.idx, v.value, v.unique);
// #endif
// 
//          // Protection. Observed this condition. Why? Supposed to be linear timestamps.
//          if(found && evframe < frame)
//          {
//             fprintf(stderr, 
//               "LV2SynthIF::getData *** Error: Event out of order: evframe:%lu < frame:%lu idx:%lu val:%f unique:%d syncFrame:%u nframes:%u v.frame:%lu\n",
//               evframe, frame, v.idx, v.value, v.unique, syncFrame, nframes, v.frame);
// 
//             // No choice but to ignore it.
//             _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
//             continue;
//          }
// 
//          if(evframe >= nframes                                                         // Next events are for a later period.
//               || (!usefixedrate && !found && !v.unique && (evframe - sample >= nsamp)) // Next events are for a later run in this period. (Autom took prio.)
//               || (found && !v.unique && (evframe - sample >= min_per))                 // Eat up events within minimum slice - they're too close.
//               || (usefixedrate && found && v.unique && v.idx == index))                // Fixed rate and must reply to all.
//          
//          {
//             break;
//          }
// 
// //          _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
// 
//          found = true;
//          frame = evframe;
//          index = v.idx;
// 
//          if(index >= _inportsControl) // Sanity check.
//          {
//             _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
//             break;
//          }
// 
//          //don't process freewheel port
//          if(_synth->_hasFreeWheelPort && _synth->_freeWheelPortIndex == index)
//          {
//             _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
//             continue;
//          }
// 
//          if(ports == 0)                     // Don't bother if not 'running'.
//          {
//             _controls[index].val = v.value;   // Might as well at least update these.
//          }
//          else
//          {
//             CtrlInterpolate *ci = &_controls[index].interp;
//             // Tell it to stop the current ramp at this frame, when it does stop, set this value:
//             ci->eFrame = frame;
//             ci->eVal   = v.value;
//             ci->eStop  = true;
//          }
// 
//          // Need to update the automation value, otherwise it overwrites later with the last automation value.
//          if(plug_id != -1)
//          {
//             synti->setPluginCtrlVal(genACnum(plug_id, index), v.value);
//          }
//          if(v.fromGui) //don't send gui control changes back
//          {
//             _state->lastControls [index] = v.value;
//             _state->controlsMask [index] = false;
//          }
//          _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
//       }
// 
//       if(found && !usefixedrate)  // If a control FIFO item was found, takes priority over automation controller stream.
//       {
//          nsamp = frame - sample;
//       }
// 
// 
//       if(sample + nsamp > nframes)         // Safety check.
//       {
//          nsamp = nframes - sample;
//       }
// 
//       // TODO: Don't allow zero-length runs. This could/should be checked in the control loop instead.
//       // Note this means it is still possible to get stuck in the top loop (at least for a while).
//       if(nsamp != 0)
//       {
//          LV2Synth::lv2audio_preProcessMidiPorts(_state, nsamp);
// 
//         // Get the state of the stop flag.
//         const bool do_stop = synti->stopFlag();
// 
//         MidiPlayEvent buf_ev;
//         
//         // Transfer the user lock-free buffer events to the user sorted multi-set.
//         // False = don't use the size snapshot, but update it.
//         const unsigned int usr_buf_sz = synti->eventBuffers(MidiDevice::UserBuffer)->getSize(false);
//         for(unsigned int i = 0; i < usr_buf_sz; ++i)
//         {
//           if(synti->eventBuffers(MidiDevice::UserBuffer)->get(buf_ev))
//             synti->_outUserEvents.insert(buf_ev);
//         }
//         
//         // Transfer the playback lock-free buffer events to the playback sorted multi-set.
//         const unsigned int pb_buf_sz = synti->eventBuffers(MidiDevice::PlaybackBuffer)->getSize(false);
//         for(unsigned int i = 0; i < pb_buf_sz; ++i)
//         {
//           // Are we stopping? Just remove the item.
//           if(do_stop)
//             synti->eventBuffers(MidiDevice::PlaybackBuffer)->remove();
//           // Otherwise get the item.
//           else if(synti->eventBuffers(MidiDevice::PlaybackBuffer)->get(buf_ev))
//             synti->_outPlaybackEvents.insert(buf_ev);
//         }
//     
//         // Are we stopping?
//         if(do_stop)
//         {
//           // Transport has stopped, purge ALL further scheduled playback events now.
//           synti->_outPlaybackEvents.clear();
//           // Reset the flag.
//           synti->setStopFlag(false);
//         }
//         
//         iMPEvent impe_pb = synti->_outPlaybackEvents.begin();
//         iMPEvent impe_us = synti->_outUserEvents.begin();
//         bool using_pb;
//     
//         while(1)
//         {
//           if(impe_pb != synti->_outPlaybackEvents.end() && impe_us != synti->_outUserEvents.end())
//             using_pb = *impe_pb < *impe_us;
//           else if(impe_pb != synti->_outPlaybackEvents.end())
//             using_pb = true;
//           else if(impe_us != synti->_outUserEvents.end())
//             using_pb = false;
//           else break;
//           
//           const MidiPlayEvent& e = using_pb ? *impe_pb : *impe_us;
// 
//           #ifdef LV2_DEBUG
//           fprintf(stderr, "LV2SynthIF::getData eventFifos event time:%d\n", e.time());
//           #endif
// 
//           if(e.time() >= (sample + nsamp + syncFrame))
//             break;
// 
//           if(ports != 0)  // Don't bother if not 'running'.
//           {
//             // Time-stamp the event.
//             unsigned int ft = (e.time() < syncFrame) ? 0 : e.time() - syncFrame;
//             ft = (ft < sample) ? 0 : ft - sample;
// 
//             if(ft >= nsamp)
//             {
//               fprintf(stderr, "LV2SynthIF::getData: eventFifos event time:%d out of range. pos:%d syncFrame:%u ft:%u sample:%lu nsamp:%lu\n", 
//                       e.time(), pos, syncFrame, ft, sample, nsamp);
//               ft = nsamp - 1;
//             }
//             if(processEvent(e, evBuf, ft))
//             {
//               
//             }
//           }
// 
//           // Done with ring buffer's event. Remove it.
//           // C++11.
//           if(using_pb)
//             impe_pb = synti->_outPlaybackEvents.erase(impe_pb);
//           else
//             impe_us = synti->_outUserEvents.erase(impe_us);
//         }
//          
// 
//          if(ports != 0)  // Don't bother if not 'running'.
//          {
// 
//             //connect ports
//             for(size_t j = 0; j < _inports; ++j)
//             {
//                if(used_in_chan_array [j])
//                {
//                   lilv_instance_connect_port(_handle, _audioInPorts [j].index, _audioInBuffers [j] + sample);
//                }
//                else
//                {
//                   lilv_instance_connect_port(_handle, _audioInPorts [j].index, _audioInSilenceBuf + sample);
//                }
//             }
// 
//             for(size_t j = 0; j < nop; ++j)
//             {
//                lilv_instance_connect_port(_handle, _audioOutPorts [j].index, buffer [j] + sample);
//             }
// 
//             for(size_t j = nop; j < _outports; j++)
//             {
//                lilv_instance_connect_port(_handle, _audioOutPorts [j].index, _audioOutBuffers [j] + sample);
//             }
// 
//             for(size_t j = 0; j < _inportsControl; ++j)
//             {
//                uint32_t idx = _controlInPorts [j].index;
//                if(_state->pluginCVPorts [idx] != nullptr)
//                {
//                   float cvVal = _controls [j].val;
//                   for(size_t jj = 0; jj < nsamp; ++jj)
//                   {
//                      _state->pluginCVPorts [idx] [jj + sample] = cvVal;
//                   }
//                   lilv_instance_connect_port(_handle, idx, _state->pluginCVPorts [idx] + sample);
//                }
//             }
// #ifdef LV2_DEBUG_PROCESS
//             //dump atom sequence events to stderr
//             if(evBuf)
//             {
//                evBuf->dump();
//             }
// #endif
// 
//             lilv_instance_run(_handle, nsamp);
//             //notify worker that this run() finished
//             if(_state->wrkIface && _state->wrkIface->end_run)
//                _state->wrkIface->end_run(lilv_instance_get_handle(_handle));
//             //notify worker about processed data (if any)
//             if(_state->wrkIface && _state->wrkIface->work_response && _state->wrkEndWork)
//             {
//                _state->wrkIface->work_response(lilv_instance_get_handle(_handle), _state->wrkDataSize, _state->wrkDataBuffer);
//                _state->wrkDataSize = 0;
//                _state->wrkDataBuffer = nullptr;
//                _state->wrkEndWork = false;
//             }
// 
//             LV2Synth::lv2audio_postProcessMidiPorts(_state, nsamp);
// 
// 
//          }
// 
//          sample += nsamp;
//       }
// 
//       ++cur_slice; // Slice is done. Moving on to any next slice now...
//    }
// 
//    return true;
// }
// 
// void LV2SynthIF::getNativeGeometry(int *x, int *y, int *w, int *h) const
// {
//    if(_state->pluginWindow != nullptr && !_state->hasExternalGui)
//    {
//       QRect g = _state->pluginWindow->geometry();
//       if(x) *x = g.x();
//       if(y) *y = g.y();
//       if(w) *w = g.width();
//       if(h) *h = g.height();
//       return;
//    }
// 
//    // Fall back to blank geometry.
//    SynthIF::getNativeGeometry(x, y, w, h);
// }
// 
// double LV2SynthIF::getParameter(long unsigned int n) const
// {
//    if(n >= _inportsControl)
//    {
//       std::cout << "LV2SynthIF::getParameter param number " << n << " out of range of ports: " << _inportsControl << std::endl;
//       return 0.0;
//    }
// 
//    if(!_controls)
//    {
//       return 0.0;
//    }
// 
//    return _controls[n].val;
// }
// 
// double LV2SynthIF::getParameterOut(long unsigned int n) const
// {
//    if(n >= _outportsControl)
//    {
//       std::cout << "LV2SynthIF::getParameterOut param number " << n << " out of range of ports: " << _outportsControl << std::endl;
//       return 0.0;
//    }
// 
//    if(!_controlsOut)
//    {
//       return 0.0;
//    }
// 
//    return _controlsOut[n].val;
// 
// }
// 
// 
// QString LV2SynthIF::getPatchName(int /* ch */, int prog, bool) const
// {
//     uint32_t program = prog & 0xff;
//     uint32_t lbank   = (prog >> 8) & 0xff;
//     uint32_t hbank   = (prog >> 16) & 0xff;
// 
//     if (program > 127)  // Map "dont care" to 0
//           program = 0;
//     if (lbank > 127)
//           lbank = 0;
//     if (hbank > 127)
//           hbank = 0;
//     const uint32_t patch = (hbank << 16) | (lbank << 8) | program;
//     
//    std::map<uint32_t, uint32_t>::iterator itPrg = _state->prg2index.find(patch);
//    if(itPrg == _state->prg2index.end())
//       return QString("?");
//    uint32_t index = itPrg->second;
//    std::map<uint32_t, lv2ExtProgram>::iterator itIndex = _state->index2prg.find(index);
//    if(itIndex == _state->index2prg.end())
//       return QString("?");
//    return QString(itIndex->second.name);
// 
// }
// 
// void LV2SynthIF::guiHeartBeat()
// {
//    //check for pending song dirty status
//    if(_state->songDirtyPending){      
//       MusEGlobal::song->setDirty();
//       _state->songDirtyPending = false;
//    }
// 
// }
// 
// bool LV2SynthIF::hasGui() const
// {
//    return true;
// }
// 
// bool LV2SynthIF::hasNativeGui() const
// {
//    return (_synth->_pluginUiTypes.size() > 0);
// }
// 
// bool LV2SynthIF::nativeGuiVisible() const
// {
//    if(_state != nullptr)
//    {
//       if(_state->hasExternalGui)
//       {
//          return (_state->widget != nullptr);
//       }
//       else if(_state->hasGui && _state->widget != nullptr)
//       {
//          return ((QWidget *)_state->widget)->isVisible();
//       }
//    }
// 
//    return false;
// }
// 
// void LV2SynthIF::populatePatchPopup(MusEGui::PopupMenu *menu, int, bool)
// {
//    LV2Synth::lv2prg_updatePrograms(_state);
//    menu->clear();
//    MusEGui::PopupMenu *subMenuPrograms = new MusEGui::PopupMenu(menu->parent());
//    subMenuPrograms->setTitle(QObject::tr("Midi programs"));
//    subMenuPrograms->setIcon(QIcon(*MusEGui::pianoNewIcon));
//    menu->addMenu(subMenuPrograms);
//    MusEGui::PopupMenu *subMenuPresets = new MusEGui::PopupMenu(menu->parent());
//    subMenuPresets->setTitle(QObject::tr("Presets"));
//    menu->addMenu(subMenuPresets);
// 
//    //First: fill programs submenu
//    std::map<int, MusEGui::PopupMenu *> submenus;
//    std::map<uint32_t, lv2ExtProgram>::iterator itIndex;
//    for(itIndex = _state->index2prg.begin(); itIndex != _state->index2prg.end(); ++itIndex)
//    {
//       const lv2ExtProgram &extPrg = itIndex->second;
//       uint32_t hb = extPrg.bank >> 8;
//       uint32_t lb = extPrg.bank & 0xff;
//       // 16384 banks arranged as 128 hi and lo banks each with up to the first 128 programs supported.
//       if(hb > 127 || lb > 127 || extPrg.prog > 127)
//          continue;
//       hb &= 0x7f;
//       lb &= 0x7f;
//       const uint32_t patch_bank = (hb << 8) | lb;
//       const uint32_t patch = (patch_bank << 8) | extPrg.prog;
//       
//       std::map<int, MusEGui::PopupMenu *>::iterator itS = submenus.find(patch_bank);
//       MusEGui::PopupMenu *submenu= nullptr;
//       if(itS != submenus.end())
//       {
//           submenu = itS->second;
//       }
//       else
//       {
//           submenu = new MusEGui::PopupMenu(menu->parent());
//           submenu->setTitle(QString("Bank #") + QString::number(extPrg.bank + 1));
//           subMenuPrograms->addMenu(submenu);
//           submenus.insert(std::make_pair(patch_bank, submenu));
//       }
// 
//       QAction *act = submenu->addAction(extPrg.name);
//       act->setData(patch);
//    }
// 
//    //Second:: Fill presets submenu
//    LV2Synth::lv2state_populatePresetsMenu(_state, subMenuPresets);
// }
// 
// void LV2SynthIF::preProcessAlways()
// {
// 
// }
// 
// MidiPlayEvent LV2SynthIF::receiveEvent()
// {
//    return MidiPlayEvent();
// 
// }
// 
// void LV2SynthIF::setNativeGeometry(int x, int y, int w, int h)
// {
//    // Store the native geometry.
//    SynthIF::setNativeGeometry(x, y, w, h);
//   
//    if(_state->pluginWindow && !_state->hasExternalGui)
//    {
//       //_state->pluginWindow->move(x, y);
//       //don't resize lv2 uis - this is handles at plugin level
//       //_uiState->pluginWindow->resize(w, h);
//       
// #ifdef QT_SHOW_POS_BUG_WORKAROUND
//       // Because of the bug, no matter what we must supply a position,
//       //  even upon first showing...
//       
//       // Check sane size.
//       if(w == 0)
//         w = _state->pluginWindow->sizeHint().width();
//       if(h == 0)
//         h = _state->pluginWindow->sizeHint().height();
// 
//       // No size hint? Try minimum size.
//       if(w == 0)
//         w = _state->pluginWindow->minimumSize().width();
//       if(h == 0)
//         h = _state->pluginWindow->minimumSize().height();
// 
//       // Fallback.
//       if(w == 0)
//         w = 400;
//       if(h == 0)
//         h = 300;
//       
//       _state->pluginWindow->setGeometry(x, y, w, h);
//       
// #else    
//       
//       // If the saved geometry is valid, use it.
//       // Otherwise this is probably the first time showing,
//       //  so do not set a geometry - let Qt pick one 
//       //  (using auto-placement and sizeHint).
//       if(!(x == 0 && y == 0 && w == 0 && h == 0))
//       {
//         // Check sane size.
//         if(w == 0)
//           w = _state->pluginWindow->sizeHint().width();
//         if(h == 0)
//           h = _state->pluginWindow->sizeHint().height();
//         
//         // No size hint? Try minimum size.
//         if(w == 0)
//           w = _state->pluginWindow->minimumSize().width();
//         if(h == 0)
//           h = _state->pluginWindow->minimumSize().height();
// 
//         // Fallback.
//         if(w == 0)
//           w = 400;
//         if(h == 0)
//           h = 300;
//         
//         _state->pluginWindow->setGeometry(x, y, w, h);
//       }
// #endif
//    }
// }
// 
// void LV2SynthIF::setParameter(long unsigned int idx, double value)
// {
//    addScheduledControlEvent(idx, value, MusEGlobal::audio->curFrame());
// }
// 
// void LV2SynthIF::showNativeGui(bool bShow)
// {
//    if(track() != nullptr)
//    {
//       if(_state->human_id != nullptr)
//       {
//          free(_state->human_id);
//       }
// 
//       _state->extHost.plugin_human_id = _state->human_id = strdup((track()->name() + QString(": ") + name()).toUtf8().constData());
//    }
// 
//   LV2Synth::lv2ui_ShowNativeGui(_state, bShow);
// }
// 
// void LV2SynthIF::write(int level, Xml &xml) const
// {
//    LV2Synth::lv2conf_write(_state, level, xml);
// }
// 
// void LV2SynthIF::setCustomData(const std::vector< QString > &customParams)
// {
//    LV2Synth::lv2conf_set(_state, customParams);
// }
// 
// 
// double LV2SynthIF::param(long unsigned int i) const
// {
//    return getParameter(i);
// }
// 
// long unsigned int LV2SynthIF::parameters() const
// {
//    return _inportsControl;
// }
// 
// long unsigned int LV2SynthIF::parametersOut() const
// {
//    return _outportsControl;
// }
// 
// const char *LV2SynthIF::paramName(long unsigned int i)
// {
//    return _controlInPorts [i].cName;
// }
// 
// const char *LV2SynthIF::paramOutName(long unsigned int i)
// {
//    return _controlOutPorts [i].cName;
// }
// 
// CtrlValueType LV2SynthIF::ctrlValueType(unsigned long i) const
// {
//    CtrlValueType vt = VAL_LINEAR;
//    std::map<uint32_t, uint32_t>::iterator it = _synth->_idxToControlMap.find(i);
//    assert(it != _synth->_idxToControlMap.end());
//    i = it->second;
//    assert(i < _inportsControl);
// 
//    switch(_synth->_controlInPorts [i].cType)
//    {
//    case LV2_PORT_CONTINUOUS:
//       vt = VAL_LINEAR;
//       break;
//    case LV2_PORT_DISCRETE:
//    case LV2_PORT_INTEGER:
//       vt = VAL_INT;
//       break;
//    case LV2_PORT_LOGARITHMIC:
//       vt = VAL_LOG;
//       break;
//    case LV2_PORT_TRIGGER:
//       vt = VAL_BOOL;
//       break;
//    default:
//       break;
//    }
// 
//    return vt;
// 
// }
// 
// CtrlList::Mode LV2SynthIF::ctrlMode(unsigned long i) const
// {
//    std::map<uint32_t, uint32_t>::iterator it = _synth->_idxToControlMap.find(i);
//    assert(it != _synth->_idxToControlMap.end());
//    i = it->second;
//    assert(i < _inportsControl);
// 
//    return ((_synth->_controlInPorts [i].cType == LV2_PORT_CONTINUOUS)
//             ||(_synth->_controlInPorts [i].cType == LV2_PORT_LOGARITHMIC)) ? CtrlList::INTERPOLATE : CtrlList::DISCRETE;
// }
// 
// LADSPA_PortRangeHint LV2SynthIF::range(unsigned long i)
// {
//    assert(i < _inportsControl);
//    LADSPA_PortRangeHint hint;
//    hint.HintDescriptor = 0;
//    hint.LowerBound = _controlInPorts [i].minVal;
//    hint.UpperBound = _controlInPorts [i].maxVal;
// 
//    if(hint.LowerBound == hint.LowerBound)
//    {
//       hint.HintDescriptor |= LADSPA_HINT_BOUNDED_BELOW;
//    }
// 
//    if(hint.UpperBound == hint.UpperBound)
//    {
//       hint.HintDescriptor |= LADSPA_HINT_BOUNDED_ABOVE;
//    }
// 
//    return hint;
// }
// 
// LADSPA_PortRangeHint LV2SynthIF::rangeOut(unsigned long i)
// {
//    assert(i < _outportsControl);
//    LADSPA_PortRangeHint hint;
//    hint.HintDescriptor = 0;
//    hint.LowerBound = _controlOutPorts [i].minVal;
//    hint.UpperBound = _controlOutPorts [i].maxVal;
// 
//    if(hint.LowerBound == hint.LowerBound)
//    {
//       hint.HintDescriptor |= LADSPA_HINT_BOUNDED_BELOW;
//    }
// 
//    if(hint.UpperBound == hint.UpperBound)
//    {
//       hint.HintDescriptor |= LADSPA_HINT_BOUNDED_ABOVE;
//    }
// 
//    return hint;
// 
// }
// 
// 
// 
// double LV2SynthIF::paramOut(long unsigned int i) const
// {
//    return getParameterOut(i);
// }
// 
// void LV2SynthIF::setParam(long unsigned int i, double val)
// {
//    setParameter(i, val);
// }
// 
// void LV2SynthIF::enableController(unsigned long i, bool v)  { _controls[i].enCtrl = v; }
// bool LV2SynthIF::controllerEnabled(unsigned long i) const   { return _controls[i].enCtrl; }
// void LV2SynthIF::enableAllControllers(bool v)
// {
//   if(!_synth)
//     return;
//   for(unsigned long i = 0; i < _inportsControl; ++i)
//     _controls[i].enCtrl = v;
// }
// void LV2SynthIF::updateControllers() { }
// 
// void LV2SynthIF::populatePresetsMenu(MusEGui::PopupMenu *menu)
// {
//    LV2Synth::lv2state_populatePresetsMenu(_state, menu);
// }
// 
// void LV2SynthIF::applyPreset(void *preset)
// {
//    LV2Synth::lv2state_applyPreset(_state, static_cast<LilvNode *>(preset));
// }
// 
// 
// 
// void LV2SynthIF::writeConfiguration(int level, Xml &xml)
// {
//    MusECore::SynthIF::writeConfiguration(level, xml);
// }
// 
// bool LV2SynthIF::readConfiguration(Xml &xml, bool readPreset)
// {
//    return MusECore::SynthIF::readConfiguration(xml, readPreset);
// }
// 
// void LV2PluginWrapper_Window::hideEvent(QHideEvent *e)
// {
//   if(_state->plugInst != nullptr)
//     _state->plugInst->saveNativeGeometry(geometry().x(), geometry().y(), geometry().width(), geometry().height());
//   else if(_state->sif != nullptr)
//     _state->sif->saveNativeGeometry(geometry().x(), geometry().y(), geometry().width(), geometry().height());
//   
//   e->ignore();
//   QMainWindow::hideEvent(e);
// }
//       
// void LV2PluginWrapper_Window::showEvent(QShowEvent *e)
// {
//   int x = 0, y = 0, w = 0, h = 0;
//   if(_state->plugInst != nullptr)
//     _state->plugInst->savedNativeGeometry(&x, &y, &w, &h);
//   else if(_state->sif != nullptr)
//     _state->sif->savedNativeGeometry(&x, &y, &w, &h);
//   
// #ifdef QT_SHOW_POS_BUG_WORKAROUND
//   // Because of the bug, no matter what we must supply a position,
//   //  even upon first showing...
//   
//   // Check sane size.
//   if(w == 0)
//     w = sizeHint().width();
//   if(h == 0)
//     h = sizeHint().height();
//   
//   // No size hint? Try minimum size.
//   if(w == 0)
//     w = minimumSize().width();
//   if(h == 0)
//     h = minimumSize().height();
// 
//   // Fallback.
//   if(w == 0)
//     w = 400;
//   if(h == 0)
//     h = 300;
//   
//   setGeometry(x, y, w, h);
//   
// #else    
//   
//   // If the saved geometry is valid, use it.
//   // Otherwise this is probably the first time showing,
//   //  so do not set a geometry - let Qt pick one 
//   //  (using auto-placement and sizeHint).
//   if(!(x == 0 && y == 0 && w == 0 && h == 0))
//   {
//     // Check sane size.
//     if(w == 0)
//       w = sizeHint().width();
//     if(h == 0)
//       h = sizeHint().height();
//     
//     // No size hint? Try minimum size.
//     if(w == 0)
//       w = minimumSize().width();
//     if(h == 0)
//       h = minimumSize().height();
//     
//     // Fallback.
//     if(w == 0)
//       w = 400;
//     if(h == 0)
//       h = 300;
//     
//     setGeometry(x, y, w, h);
//   }
// #endif
//     
//   // Convenience: If the window was minimized, restore it.
//   if(isMinimized())
//     setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
//   
//   e->ignore();
//   QMainWindow::showEvent(e);
// }
// 
// void LV2PluginWrapper_Window::closeEvent(QCloseEvent *event)
// {
//    assert(_state != nullptr);
//    event->accept();
// 
//    stopUpdateTimer();
// 
//    //if(_state->gtk2Plug != nullptr)
//    //{
//       if(_state->pluginQWindow != nullptr)
//       {
//         _state->pluginQWindow->setParent(NULL);
//         delete _state->pluginQWindow;
//         _state->pluginQWindow = nullptr;
//       }
//    //}
// 
//    if(_state->deleteLater)
//    {
//       LV2Synth::lv2state_FreeState(_state);
// 
//    }
//    else
//    {
//       //_state->uiTimer->stopNextTime(false);
//       _state->widget = nullptr;
//       _state->pluginWindow = nullptr;
//       _state->uiDoSelectPrg = false;
//       _state->uiPrgIface = nullptr;
// 
//       LV2Synth::lv2ui_FreeDescriptors(_state);
//    }
// 
//    // Reset the flag, just to be sure.
//    _state->uiIsOpening = false;
//    
//    // The widget is automatically deleted by use of the 
//    //  WA_DeleteOnClose attribute in the constructor.
// }
// 
// void LV2PluginWrapper_Window::stopUpdateTimer()
// {
//    if(updateTimer.isActive())
//       updateTimer.stop();
//    while(updateTimer.isActive())
//    {
//       QCoreApplication::processEvents();
//    }
// }
// 
// 
// #ifdef LV2_GUI_USE_QWIDGET
// LV2PluginWrapper_Window::LV2PluginWrapper_Window(LV2PluginWrapper_State *state, 
//                                                  QWidget *parent, 
//                                                  Qt::WindowFlags flags)
//  : QWidget(parent, flags), _state ( state ), _closing(false)
// #else
// LV2PluginWrapper_Window::LV2PluginWrapper_Window(LV2PluginWrapper_State *state, 
//                                                  QWidget *parent, 
//                                                  Qt::WindowFlags flags)
//  : QMainWindow(parent, flags), _state ( state ), _closing(false)
// #endif
// {
//    // Automatically delete the wiget when it closes.
//    setAttribute(Qt::WA_DeleteOnClose, true);
//    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateGui()));
//    connect(this, SIGNAL(makeStopFromGuiThread()), this, SLOT(stopFromGuiThread()));
//    connect(this, SIGNAL(makeStartFromGuiThread()), this, SLOT(startFromGuiThread()));
// }
// 
// LV2PluginWrapper_Window::~LV2PluginWrapper_Window()
// {
// #ifdef DEBUG_LV2
//    std::cout << "LV2PluginWrapper_Window::~LV2PluginWrapper_Window()" << std::endl;
// #endif
// }
// 
// void LV2PluginWrapper_Window::startNextTime()
// {
//    emit makeStartFromGuiThread();
// }
// 
// 
// 
// 
// void LV2PluginWrapper_Window::stopNextTime()
// {
//    setClosing(true);
//    emit makeStopFromGuiThread();
// }
// 
// void LV2PluginWrapper_Window::updateGui()
// {
//    if(_state->deleteLater || _closing)
//    {
//       stopNextTime();
//       return;
//    }
//    LV2Synth::lv2ui_SendChangedControls(_state);
// 
//    //send program change if any
//    // Force send if re-opening.
//    if(_state->uiIsOpening || _state->uiDoSelectPrg)
//    {
//       _state->uiDoSelectPrg = false;
//       if(_state->uiPrgIface != nullptr && (_state->uiPrgIface->select_program != nullptr || _state->uiPrgIface->select_program_for_channel != nullptr))
//       {
//          if(_state->newPrgIface)
//             _state->uiPrgIface->select_program_for_channel(lilv_instance_get_handle(_state->handle), _state->uiChannel, (uint32_t)_state->uiBank, (uint32_t)_state->uiProg);
//          else
//             _state->uiPrgIface->select_program(lilv_instance_get_handle(_state->handle), (uint32_t)_state->uiBank, (uint32_t)_state->uiProg);
//       }
//    }
// 
//    // Reset the flag.
//    _state->uiIsOpening = false;
//    
//    //call ui idle callback if any
//    if(_state->uiIdleIface != nullptr)
//    {
//       int iRet = _state->uiIdleIface->idle(_state->uiInst);
//       if(iRet != 0) // ui don't want us to call it's idle callback any more
//          _state->uiIdleIface = nullptr;
//    }
// 
//    if(_state->hasExternalGui)
//    {
//       LV2_EXTERNAL_UI_RUN((LV2_External_UI_Widget *)_state->widget);
//    }
// 
//    //if(_closing)
//    //stopNextTime();
// }
// 
// void LV2PluginWrapper_Window::stopFromGuiThread()
// {
//    stopUpdateTimer();
//    emit close();
// }
// 
// void LV2PluginWrapper_Window::startFromGuiThread()
// {
//    stopUpdateTimer();
//    updateTimer.start(1000/30);
// }
// 
// 
// LV2PluginWrapper::LV2PluginWrapper(LV2Synth *s, PluginFeatures reqFeatures)
// {
//    _synth = s;
// 
//    _requiredFeatures = reqFeatures;
//    
//    _fakeLd.Label      = strdup(_synth->name().toUtf8().constData());
//    _fakeLd.Name       = strdup(_synth->name().toUtf8().constData());
//    _fakeLd.UniqueID   = _synth->_uniqueID;
//    _fakeLd.Maker      = strdup(_synth->maker().toUtf8().constData());
//    _fakeLd.Copyright  = strdup(_synth->version().toUtf8().constData());
//    _isLV2Plugin = true;
//    _isLV2Synth = s->_isSynth;
//    int numPorts = _synth->_audioInPorts.size()
//                   + _synth->_audioOutPorts.size()
//                   + _synth->_controlInPorts.size()
//                   + _synth->_controlOutPorts.size()
//                   + _synth->_midiInPorts.size()
//                   + _synth->_midiOutPorts.size();
//    _fakeLd.PortCount = numPorts;
//    _fakePds = new LADSPA_PortDescriptor [numPorts];
//    memset(_fakePds, 0, sizeof(int) * numPorts);
// 
//    for(size_t i = 0; i < _synth->_audioInPorts.size(); i++)
//    {
//       _fakePds [_synth->_audioInPorts [i].index] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
//    }
// 
//    for(size_t i = 0; i < _synth->_audioOutPorts.size(); i++)
//    {
//       _fakePds [_synth->_audioOutPorts [i].index] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
//    }
// 
//    for(size_t i = 0; i < _synth->_controlInPorts.size(); i++)
//    {
//       _fakePds [_synth->_controlInPorts [i].index] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
//    }
// 
//    for(size_t i = 0; i < _synth->_controlOutPorts.size(); i++)
//    {
//       _fakePds [_synth->_controlOutPorts [i].index] = LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
//    }   
// 
//    _fakeLd.PortNames = nullptr;
//    _fakeLd.PortRangeHints = nullptr;
//    _fakeLd.PortDescriptors = _fakePds;
//    _fakeLd.Properties = 0;
//    plugin = &_fakeLd;
//    _isDssi = false;
//    _isDssiSynth = false;
//    _isVstNativePlugin = false;
//    _isVstNativeSynth = false;
// 
// #ifdef DSSI_SUPPORT
//    dssi_descr = nullptr;
// #endif
// 
//    fi = _synth->info;
//    ladspa = nullptr;
//    _handle = 0;
//    _references = 0;
//    _instNo     = 0;
//    _label = _synth->name();
//    _name = _synth->description();
//    _uniqueID = plugin->UniqueID;
//    _maker = _synth->maker();
//    _copyright = _synth->version();
// 
//    _portCount = plugin->PortCount;
// 
//    _inports = 0;
//    _outports = 0;
//    _controlInPorts = 0;
//    _controlOutPorts = 0;
// 
//    for(unsigned long k = 0; k < _portCount; ++k)
//    {
//       LADSPA_PortDescriptor pd = plugin->PortDescriptors[k];
// 
//       if(pd & LADSPA_PORT_AUDIO)
//       {
//          if(pd & LADSPA_PORT_INPUT)
//          {
//             ++_inports;
//          }
//          else if(pd & LADSPA_PORT_OUTPUT)
//          {
//             ++_outports;
//          }
//       }
//       else if(pd & LADSPA_PORT_CONTROL)
//       {
//          if(pd & LADSPA_PORT_INPUT)
//          {
//             ++_controlInPorts;
//          }
//          else if(pd & LADSPA_PORT_OUTPUT)
//          {
//             ++_controlOutPorts;
//          }
//       }
//    }
// }
// 
// LV2PluginWrapper::~LV2PluginWrapper()
// {
//    free((void*)_fakeLd.Label);
//    free((void*)_fakeLd.Name);
//    free((void*)_fakeLd.Maker);
//    free((void*)_fakeLd.Copyright);
//    delete [] _fakePds;
// }
// 
// LADSPA_Handle LV2PluginWrapper::instantiate(PluginI *plugi)
// {
//    LV2PluginWrapper_State *state = new LV2PluginWrapper_State;
//    state->inst = this;
//    state->widget = nullptr;
//    state->uiInst = nullptr;
//    state->plugInst = plugi;
//    state->_ifeatures = new LV2_Feature[SIZEOF_ARRAY(lv2Features)];
//    state->_ppifeatures = new LV2_Feature *[SIZEOF_ARRAY(lv2Features) + 1];
//    state->sif = nullptr;
//    state->synth = _synth;
//    LV2Synth::lv2state_FillFeatures(state);
// 
//    state->handle = lilv_plugin_instantiate(_synth->_handle, (double)MusEGlobal::sampleRate, state->_ppifeatures);
// 
//    if(state->handle == nullptr)
//    {
//       delete [] state->_ppifeatures;
//       delete [] state->_ifeatures;
//       return NULL;
//    }
// 
//    //_states.insert(std::pair<void *, LV2PluginWrapper_State *>(state->handle, state));
// 
//    LV2Synth::lv2state_PostInstantiate(state);
// 
//    return (LADSPA_Handle)state;
// 
// }
// 
// void LV2PluginWrapper::connectPort(LADSPA_Handle handle, long unsigned int port, float *value)
// {
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
//    lilv_instance_connect_port((LilvInstance *)state->handle, port, (void *)value);
// }
// 
// int LV2PluginWrapper::incReferences(int ref)
// {
//    _synth->incInstances(ref);
//    return _synth->instances();
// }
// void LV2PluginWrapper::activate(LADSPA_Handle handle)
// {
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
//    lilv_instance_activate((LilvInstance *) state->handle);
// }
// void LV2PluginWrapper::deactivate(LADSPA_Handle handle)
// {
//    if (handle)
//    {
//       LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
//       lilv_instance_deactivate((LilvInstance *) state->handle);
//    }
// }
// void LV2PluginWrapper::cleanup(LADSPA_Handle handle)
// {
//    if(handle != nullptr)
//    {      
//       LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
// 
//       state->deleteLater = true;
//       if(state->pluginWindow != nullptr)
//          state->pluginWindow->stopNextTime();
//       else
//          LV2Synth::lv2state_FreeState(state);
// 
//    }
// }
// 
// void LV2PluginWrapper::apply(LADSPA_Handle handle, unsigned long n)
// {
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
// 
//    LV2Synth::lv2audio_preProcessMidiPorts(state, n);
// 
//    //set freewheeling property if plugin supports it
//    if(state->synth->_hasFreeWheelPort)
//    {
//       state->plugInst->controls[_synth->_freeWheelPortIndex].val = MusEGlobal::audio->freewheel() ? 1.0f : 0.0f;
//    }
// 
//    for(size_t j = 0; j < state->plugInst->controlPorts; ++j)
//    {
//       uint32_t idx = state->synth->_controlInPorts [j].index;
//       if(state->pluginCVPorts [idx] != nullptr)
//       {
//          float cvVal = state->plugInst->controls [j].val;
//          for(size_t jj = 0; jj < n; ++jj)
//          {
//             state->pluginCVPorts [idx] [jj] = cvVal;
//          }
//          lilv_instance_connect_port(state->handle, idx, state->pluginCVPorts [idx]);
//       }
//    }
// 
//    for(size_t j = 0; j < state->plugInst->controlOutPorts; ++j)
//    {
//       uint32_t idx = state->synth->_controlOutPorts [j].index;
//       if(state->pluginCVPorts [idx] != nullptr)
//       {
//          float cvVal = state->plugInst->controlsOut [j].val;
//          for(size_t jj = 0; jj < n; ++jj)
//          {
//             state->pluginCVPorts [idx] [jj] = cvVal;
//          }
//          lilv_instance_connect_port(state->handle, idx, state->pluginCVPorts [idx]);
//       }
//    }
// 
// 
//    lilv_instance_run(state->handle, n);
//    //notify worker that this run() finished
//    if(state->wrkIface && state->wrkIface->end_run)
//       state->wrkIface->end_run(lilv_instance_get_handle(state->handle));
//    //notify worker about processes data (if any)
//    if(state->wrkIface && state->wrkIface->work_response && state->wrkEndWork)
//    {
//       state->wrkEndWork = false;
//       state->wrkIface->work_response(lilv_instance_get_handle(state->handle), state->wrkDataSize, state->wrkDataBuffer);
//       state->wrkDataSize = 0;
//       state->wrkDataBuffer = nullptr;
//    }
// 
//    LV2Synth::lv2audio_postProcessMidiPorts(state, n);
// }
// LADSPA_PortDescriptor LV2PluginWrapper::portd(unsigned long k) const
// {
//    return _fakeLd.PortDescriptors[k];
// }
// 
// LADSPA_PortRangeHint LV2PluginWrapper::range(unsigned long i)
// {
//    LADSPA_PortRangeHint hint;
//    hint.HintDescriptor = 0;
//    hint.LowerBound = _synth->_pluginControlsMin [i];
//    hint.UpperBound = _synth->_pluginControlsMax [i];
// 
//    if(hint.LowerBound == hint.LowerBound)
//    {
//       hint.HintDescriptor |= LADSPA_HINT_BOUNDED_BELOW;
//    }
// 
//    if(hint.UpperBound == hint.UpperBound)
//    {
//       hint.HintDescriptor |= LADSPA_HINT_BOUNDED_ABOVE;
//    }
// 
//    return hint;
// }
// void LV2PluginWrapper::range(unsigned long i, float *min, float *max) const
// {
//    *min = _synth->_pluginControlsMin [i];
//    *max = _synth->_pluginControlsMax [i];
// }
// 
// double LV2PluginWrapper::defaultValue(unsigned long port) const
// {
//    return _synth->_pluginControlsDefault [port];
// }
// const char *LV2PluginWrapper::portName(unsigned long i)
// {
//    return lilv_node_as_string(lilv_port_get_name(_synth->_handle, lilv_plugin_get_port_by_index(_synth->_handle, i)));
// }
// 
// CtrlValueType LV2PluginWrapper::ctrlValueType(unsigned long i) const
// {
//    CtrlValueType vt = VAL_LINEAR;
//    std::map<uint32_t, uint32_t>::iterator it = _synth->_idxToControlMap.find(i);
//    assert(it != _synth->_idxToControlMap.end());
//    i = it->second;
//    assert(i < _controlInPorts);
// 
//    switch(_synth->_controlInPorts [i].cType)
//    {
//    case LV2_PORT_CONTINUOUS:
//       vt = VAL_LINEAR;
//       break;
//    case LV2_PORT_DISCRETE:
//    case LV2_PORT_INTEGER:
//       vt = VAL_INT;
//       break;
//    case LV2_PORT_LOGARITHMIC:
//       vt = VAL_LOG;
//       break;
//    case LV2_PORT_TRIGGER:
//       vt = VAL_BOOL;
//       break;
//    default:
//       break;
//    }
// 
//    return vt;
// }
// CtrlList::Mode LV2PluginWrapper::ctrlMode(unsigned long i) const
// {
//    std::map<uint32_t, uint32_t>::iterator it = _synth->_idxToControlMap.find(i);
//    assert(it != _synth->_idxToControlMap.end());
//    i = it->second;
//    assert(i < _controlInPorts);
// 
//    return ((_synth->_controlInPorts [i].cType == LV2_PORT_CONTINUOUS)
//            ||(_synth->_controlInPorts [i].cType == LV2_PORT_LOGARITHMIC)) ? CtrlList::INTERPOLATE : CtrlList::DISCRETE;
// }
// bool LV2PluginWrapper::hasNativeGui() const
// {
//    return (_synth->_pluginUiTypes.size() > 0);
// }
// 
// void LV2PluginWrapper::showNativeGui(PluginI *p, bool bShow)
// {
//    assert(p->instances > 0);
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)p->handle [0];
// 
//    if(p->track() != nullptr)
//    {
//       if(state->human_id != nullptr)
//       {
//          free(state->human_id);
//       }
// 
//       state->extHost.plugin_human_id = state->human_id = strdup((p->track()->name() + QString(": ") + label()).toUtf8().constData());
//    }
// 
//   LV2Synth::lv2ui_ShowNativeGui(state, bShow);
// }
// 
// bool LV2PluginWrapper::nativeGuiVisible(const PluginI *p) const
// {
//    assert(p->instances > 0);
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)p->handle [0];
//    return (state->widget != nullptr);
// }
// 
// void LV2PluginWrapper::setLastStateControls(LADSPA_Handle handle, size_t index, bool bSetMask, bool bSetVal, bool bMask, float fVal)
// {
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
//    assert(state != nullptr);
// 
//    if(_controlInPorts == 0)
//       return;
// 
//    if(bSetMask)
//       state->controlsMask [index] = bMask;
// 
//    if(bSetVal)
//       state->lastControls [index] = fVal;
// 
// }
// 
// void LV2PluginWrapper::writeConfiguration(LADSPA_Handle handle, int level, Xml &xml)
// {
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
//    assert(state != nullptr);
// 
//    LV2Synth::lv2conf_write(state, level, xml);
// }
// 
// void LV2PluginWrapper::setCustomData(LADSPA_Handle handle, const std::vector<QString> &customParams)
// {
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
//    assert(state != nullptr);
// 
//    LV2Synth::lv2conf_set(state, customParams);
// }
// 
// void LV2PluginWrapper::populatePresetsMenu(PluginI *p, MusEGui::PopupMenu *menu)
// {
//    assert(p->instances > 0);
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)p->handle [0];
//    assert(state != nullptr);
// 
//    LV2Synth::lv2state_populatePresetsMenu(state, menu);
// 
// }
// 
// void LV2PluginWrapper::applyPreset(PluginI *p, void *preset)
// {
//    assert(p->instances > 0);
//    LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)p->handle [0];
//    assert(state != nullptr);
// 
//    LV2Synth::lv2state_applyPreset(state, static_cast<LilvNode *>(preset));
// 
// }
// 
// void LV2PluginWrapper_Worker::run()
// {
//    while(true)
//    {
//       _mSem.acquire(1);
//       if(_closing)
//          break;
//       makeWork();
// 
//    }
// 
// }
// 
// LV2_Worker_Status LV2PluginWrapper_Worker::scheduleWork()
// {
//    if(_mSem.available() != 0)
//       return LV2_WORKER_ERR_NO_SPACE;
//    _mSem.release(1);
//    return LV2_WORKER_SUCCESS;
// 
// }

void LV2PluginWrapper_Worker::makeWork()
{
#ifdef DEBUG_LV2
   std::cerr << "LV2PluginWrapper_Worker::makeWork" << std::endl;
#endif
   if(_state->wrkIface && _state->wrkIface->work)
   {
      const void *dataBuffer = _state->wrkDataBuffer;
      uint32_t dataSize = _state->wrkDataSize;
      _state->wrkDataBuffer = nullptr;
      _state->wrkDataSize = 0;
      if(_state->wrkIface->work(lilv_instance_get_handle(_state->handle),
//                                 LV2Synth::lv2wrk_respond,
                                Lv2Plugin::lv2wrk_respond,
                                _state,
                                dataSize,
                                dataBuffer) != LV2_WORKER_SUCCESS)
      {
         _state->wrkEndWork = false;
         _state->wrkDataBuffer = nullptr;
         _state->wrkDataSize = 0;
      }
   }

}

#ifdef LV2_EVENT_BUFFER_SUPPORT
LV2EvBuf::LV2EvBuf(bool isInput, bool oldApi, LV2_URID atomTypeSequence, LV2_URID atomTypeChunk)
   :_isInput(isInput), _oldApi(oldApi), _uAtomTypeSequence(atomTypeSequence), _uAtomTypeChunk(atomTypeChunk)
#else
LV2EvBuf::LV2EvBuf(bool isInput, LV2_URID atomTypeSequence, LV2_URID atomTypeChunk)
   :_isInput(isInput), _uAtomTypeSequence(atomTypeSequence), _uAtomTypeChunk(atomTypeChunk)
#endif
{
   if(_isInput)
   {
     // Resize and fill with initial value.
     _buffer.resize(LV2_EVBUF_SIZE, 0);
   }
   else
   {
     // Reserve the space.
     _buffer.reserve(LV2_EVBUF_SIZE);
     // Add one item, the first item.
     _buffer.assign(sizeof(LV2_Atom_Sequence), 0);
   }
   
#ifdef LV2_DEBUG
   std::cerr << "LV2EvBuf ctor: _buffer size:" << _buffer.size() << " capacity:" << _buffer.capacity() << std::endl;
#endif

   resetBuffer();
}

size_t LV2EvBuf::mkPadSize(size_t size)
{
   return (size + 7U) & (~7U);
}

void LV2EvBuf::resetPointers(bool r, bool w)
{
   if(!r && !w)
      return;
   size_t ptr = 0;
#ifdef LV2_EVENT_BUFFER_SUPPORT
   if(_oldApi)
   {
      ptr = sizeof(LV2_Event_Buffer);
   }
   else
#endif
   {
      ptr = sizeof(LV2_Atom_Sequence);
   }

   if(r)
   {
      curRPointer = ptr;
   }
   if(w)
   {
      curWPointer = ptr;
   }
}

void LV2EvBuf::resetBuffer()
{
#ifdef LV2_EVENT_BUFFER_SUPPORT
   if(_oldApi)
   {
      _evbuf = reinterpret_cast<LV2_Event_Buffer *>(&_buffer [0]);
      _evbuf->capacity = _buffer.size() - sizeof(LV2_Event_Buffer);
      _evbuf->data = reinterpret_cast<uint8_t *>(_evbuf + 1);
      _evbuf->event_count = 0;
      _evbuf->header_size = sizeof(LV2_Event_Buffer);
      _evbuf->stamp_type = LV2_EVENT_AUDIO_STAMP;
      _evbuf->size = 0;
   }
   else
#endif
   {
      _seqbuf = reinterpret_cast<LV2_Atom_Sequence *>(&_buffer [0]);
      if(!_isInput)
      {
         _seqbuf->atom.type = _uAtomTypeChunk;
         _seqbuf->atom.size = _buffer.size() - sizeof(LV2_Atom_Sequence);
      }
      else
      {
         _seqbuf->atom.type = _uAtomTypeSequence;
         _seqbuf->atom.size = sizeof(LV2_Atom_Sequence_Body);
      }
      _seqbuf->body.unit = 0;
      _seqbuf->body.pad = 0;
   }
   resetPointers(true, true);
}

bool LV2EvBuf::write(uint32_t frames, uint32_t subframes, uint32_t type, uint32_t size, const uint8_t *data)
{
   if(!_isInput)
      return false;
#ifdef LV2_EVENT_BUFFER_SUPPORT
   if(_oldApi)
   {
      size_t paddedSize = mkPadSize(sizeof(LV2_Event) + size);
      size_t resSize = curWPointer + paddedSize;
      if(resSize > _buffer.size())
      {
         std::cerr << "LV2 Event buffer overflow! frames=" << frames << ", size=" << size << std::endl;
         return false;
      }
      LV2_Event *ev = reinterpret_cast<LV2_Event *>(&_buffer [curWPointer]);
      ev->frames = frames;
      ev->subframes = subframes;
      ev->type = type;
      ev->size = size;
      memcpy((void *)(ev + 1), data, size);
      curWPointer += paddedSize;
      _evbuf->size += paddedSize;
      _evbuf->event_count++;
   }
   else
#endif
   {
      size_t paddedSize = mkPadSize(sizeof(LV2_Atom_Event) + size);
      size_t resSize = curWPointer + paddedSize;
      if(resSize > _buffer.size())
      {
         std::cerr << "LV2 Atom_Event buffer overflow! frames=" << frames << ", size=" << size << std::endl;
         return false;
      }

      LV2_Atom_Event *ev = reinterpret_cast<LV2_Atom_Event *>(&_buffer [curWPointer]);
      ev->time.frames = frames;
      ev->body.size = size;
      ev->body.type = type;
      memcpy(LV2_ATOM_BODY(&ev->body), data, size);
      _seqbuf->atom.size += paddedSize;
      curWPointer += paddedSize;
   }
   return true;
}

bool LV2EvBuf::read(uint32_t *frames, uint32_t *subframes, uint32_t *type, uint32_t *size, uint8_t **data)
{
   *frames = *subframes = *type = *size = 0;
   *data = nullptr;
   if(_isInput)
      return false;
#ifdef LV2_EVENT_BUFFER_SUPPORT
   if(_oldApi)
   {
      LV2_Event *ev = reinterpret_cast<LV2_Event *>(&_buffer [curRPointer]);
      if((_evbuf->size + sizeof(LV2_Event_Buffer) - curRPointer) < sizeof(LV2_Event))
      {
         return false;
      }
      *frames = ev->frames;
      *subframes = ev->subframes;
      *type = ev->type;
      *size = ev->size;
      *data = reinterpret_cast<uint8_t *>(ev + 1);
      size_t padSize = mkPadSize(sizeof(LV2_Event) + ev->size);
      curRPointer += padSize;
   }
   else
#endif
   {
      LV2_Atom_Event *ev = reinterpret_cast<LV2_Atom_Event *>(&_buffer [curRPointer]);
      if((_seqbuf->atom.size + sizeof(LV2_Atom_Sequence) - curRPointer) < sizeof(LV2_Atom_Event))
      {
         return false;
      }
      *frames = ev->time.frames;
      *type = ev->body.type;
      *size = ev->body.size;
      *data = reinterpret_cast<uint8_t *>(LV2_ATOM_BODY(&ev->body));
      size_t padSize = mkPadSize(sizeof(LV2_Atom_Event) + ev->body.size);
      curRPointer += padSize;

   }
   return true;
}

uint8_t *LV2EvBuf::getRawBuffer()
{
   return &_buffer [0];
}

void LV2EvBuf::dump()
{
#ifdef LV2_EVENT_BUFFER_SUPPORT
   if(_oldApi){
      return;
   }
#endif

   int n = 1;
   LV2_Atom_Sequence *b = (LV2_Atom_Sequence *)&_buffer [0];
   LV2_ATOM_SEQUENCE_FOREACH(b, s)
   {
      if(n == 1)
      {
         fprintf(stderr, "-------------- Atom seq dump START---------------\n");
      }
      fprintf(stderr, "\tSeq. no.: %d\n", n);
      fprintf(stderr, "\t\tFrames: %ld\n", (long)s->time.frames);
      fprintf(stderr, "\t\tSize: %d\n", s->body.size);
      fprintf(stderr, "\t\tType: %d\n", s->body.type);
      fprintf(stderr, "\t\tData (hex):\n");
      uint8_t *d = (uint8_t *)LV2_ATOM_BODY(&s->body);
      for(uint32_t i = 0; i < s->body.size; i++)
      {
         if((i % 10) == 0)
         {
            fprintf(stderr, "\n\t\t");
         }
         else
         {
            fprintf(stderr, " ");
         }
         fprintf(stderr, "0x%02X", d [i]);
      }
      fprintf(stderr, "\n");

      n++;
   }

   if(n > 1)
   {
      fprintf(stderr, "-------------- Atom seq dump END---------------\n\n");
   }
}

LV2SimpleRTFifo::LV2SimpleRTFifo(size_t size):
   fifoSize(size),
   itemSize(LV2_RT_FIFO_ITEM_SIZE)
{
   eventsBuffer.resize(fifoSize);
   assert(eventsBuffer.size() == fifoSize);
   readIndex = writeIndex = 0;
   for(size_t i = 0; i < fifoSize; ++i)
   {
      eventsBuffer [i].port_index = 0;
      eventsBuffer [i].buffer_size = 0;
      eventsBuffer [i].data = new char [itemSize];
   }

}

LV2SimpleRTFifo::~LV2SimpleRTFifo()
{
   for(size_t i = 0; i < fifoSize; ++i)
   {
      delete [] eventsBuffer [i].data;
   }
}

bool LV2SimpleRTFifo::put(uint32_t port_index, uint32_t size, const void *data)
{
   if(size > itemSize)
   {
#ifdef DEBUG_LV2
      std::cerr << "LV2SimpleRTFifo:put(): size("<<size<<") is too big" << std::endl;
#endif
      return false;

   }
   size_t i = writeIndex;
   bool found = false;
   do
   {
      if(eventsBuffer.at(i).buffer_size == 0)
      {
         found = true;
         break;
      }
      i++;
      i %= fifoSize;
   }
   while(i != writeIndex);

   if(!found)
   {
#ifdef DEBUG_LV2
      std::cerr << "LV2SimpleRTFifo:put(): fifo is full" << std::endl;
#endif
      return false;
   }
#ifdef DEBUG_LV2
   // std::cerr << "LV2SimpleRTFifo:put(): used index = " << i << std::endl;
#endif
   memcpy(eventsBuffer.at(i).data, data, size);
   eventsBuffer.at(i).port_index = port_index;
   __sync_fetch_and_add(&eventsBuffer.at(i).buffer_size, size);
   writeIndex = (i + 1) % fifoSize;

   return true;

}

bool LV2SimpleRTFifo::get(uint32_t *port_index, size_t *szOut, char *data_out)
{
   size_t i = readIndex;
   bool found = false;
   if(eventsBuffer.at(i).buffer_size != 0)
   {
      found = true;
   }

   if(!found)
   {
#ifdef DEBUG_LV2
      //std::cerr << "LV2SimpleRTFifo:get(): fifo is empty" << std::endl;
#endif
      return false;
   }
#ifdef DEBUG_LV2
   //std::cerr << "LV2SimpleRTFifo:get(): used index = " << i << std::endl;
#endif
   *szOut = eventsBuffer.at(i).buffer_size;
   *port_index = eventsBuffer [i].port_index;
   memcpy(data_out, eventsBuffer [i].data, *szOut);
   __sync_fetch_and_and(&eventsBuffer.at(i).buffer_size, 0);
   readIndex = (i + 1) % fifoSize;
   return true;
}

LV2UridBiMap::LV2UridBiMap() : nextId ( 1 ) {_map.clear(); _rmap.clear();}

LV2UridBiMap::~LV2UridBiMap()
{
   LV2_SYNTH_URID_MAP::iterator it = _map.begin();
   for(;it != _map.end(); ++it)
   {
      free((void*)(*it).first);
   }
}

LV2_URID LV2UridBiMap::map(const char *uri)
{
   std::pair<LV2_SYNTH_URID_MAP::iterator, bool> p;
   uint32_t id;
   idLock.lock();
   LV2_SYNTH_URID_MAP::iterator it = _map.find(uri);
   if(it == _map.end())
   {
      const char *mUri = strdup(uri);
      p = _map.insert ( std::make_pair ( mUri, nextId ) );
      _rmap.insert ( std::make_pair ( nextId, mUri ) );
      nextId++;
      id = p.first->second;
   }
   else
      id = it->second;
   idLock.unlock();
   return id;

}

const char *LV2UridBiMap::unmap(uint32_t id)
{
   LV2_SYNTH_URID_RMAP::iterator it = _rmap.find ( id );
   if ( it != _rmap.end() ) {
      return it->second;
   }

   return NULL;
}


//---------------------------------
//  Lv2PluginI
//---------------------------------

void Lv2PluginI::init()
{
  _handle = 0;
//   _features = nullptr;
//   _ppfeatures = nullptr;
  _options = nullptr;
  _state = nullptr;

//   _state = new LV2PluginWrapper_State;
// //   _state->inst = nullptr;
//   _state->widget = nullptr;
//   _state->uiInst = nullptr;
//   _state->plugInst = nullptr;
//   _state->_ifeatures = new LV2_Feature[SIZEOF_ARRAY(lv2Features)];
//   _state->_ppifeatures = new LV2_Feature *[SIZEOF_ARRAY(lv2Features) + 1];
//   _state->sif = this;
//   //_state->synth = _synth;
// 
//   _features = new LV2_Feature[SIZEOF_ARRAY(lv2Features)];
//   _ppfeatures = new LV2_Feature *[SIZEOF_ARRAY(lv2Features) + 1];   


//    _sampleRate = (double)MusEGlobal::sampleRate;
//    _fSampleRate = (float)MusEGlobal::sampleRate;
//    _iSampleRate = samplerate;
//    _sampleRate = (double)_iSampleRate;
//    _fSampleRate = (float)_iSampleRate;
// 
//    _segSize = segment_size;
/*   
   //prepare features and options arrays
   LV2_Options_Option _tmpl_options [] =
   {
      {LV2_OPTIONS_INSTANCE, 0, uridBiMap.map(LV2_P_SAMPLE_RATE), sizeof(float), uridBiMap.map(LV2_ATOM__Float), &_fSampleRate},
      {LV2_OPTIONS_INSTANCE, 0, uridBiMap.map(LV2_P_MIN_BLKLEN), sizeof(int32_t), uridBiMap.map(LV2_ATOM__Int), &Lv2Plugin::minBlockSize}, //&_segSize},
      {LV2_OPTIONS_INSTANCE, 0, uridBiMap.map(LV2_P_MAX_BLKLEN), sizeof(int32_t), uridBiMap.map(LV2_ATOM__Int), &_segSize},
      {LV2_OPTIONS_INSTANCE, 0, uridBiMap.map(LV2_P_NOM_BLKLEN), sizeof(int32_t), uridBiMap.map(LV2_ATOM__Int), &_segSize},
      {LV2_OPTIONS_INSTANCE, 0, uridBiMap.map(LV2_P_SEQ_SIZE), sizeof(int32_t), uridBiMap.map(LV2_ATOM__Int), &_segSize},
      {LV2_OPTIONS_INSTANCE, 0, uridBiMap.map(LV2_CORE__sampleRate), sizeof(double), uridBiMap.map(LV2_ATOM__Double), &_sampleRate},
      {LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, nullptr}

   };

   _options = new LV2_Options_Option[SIZEOF_ARRAY(_tmpl_options)]; // last option is NULLs

   for(uint32_t i = 0; i < SIZEOF_ARRAY(_tmpl_options); i++)
   {
      _options [i] = _tmpl_options [i];
   }*/






  
  //LV2Synth::lv2state_FillFeatures(_state);
  //lv2state_FillFeatures(_state);
  //lv2_FillFeatures(_features, _ppfeatures, options);
  //Lv2Plugin::lv2_FillFeatures(_features, _ppfeatures, options);
}

Lv2PluginI::Lv2PluginI()
{
  init();
}

Lv2PluginI::~Lv2PluginI()
{
  if(_plugin) {
    // Deactivate is pure virtual, it cannot be 
    //  called from the base destructor. Do it here.
    deactivate();
    _plugin->incReferences(-1);
  }
  
  if(_handle)
    delete[] _handle;
  
  
  
// TODO: pre  
//    Lv2Plugin::lv2state_UnloadLoadPresets(this);

//    if(_ppfeatures)
//    {
//       delete [] _ppfeatures;
//       _ppfeatures = nullptr;
//    }
// 
//    if(_features)
//    {
//       delete [] _features;
//       _features = nullptr;
//    }

   if(_options)
   {
      delete [] _options;
      _options = nullptr;
   }

// TODO: UI
//    if(_uis != nullptr)
//    {
//       lilv_uis_free(_uis);
//       _uis = nullptr;
//    }
 
 
 
 
   if(_state != nullptr)
   {
     // TODO: Be sure the state gets deleted later, wherever that is...
      _state->deleteLater = true;
      
      //_uiState->uiTimer->stopNextTime(false);

// TODO: UI
//       if(_state->pluginWindow != nullptr)
//          _state->pluginWindow->stopNextTime();
//       else
         Lv2Plugin::lv2state_FreeState(_state);
         
      _state = nullptr;
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

   if(_audioInSilenceBuf)
     free(_audioInSilenceBuf);
  
   if(_audioInBuffers)
   {
      delete [] _audioInBuffers;
      _audioInBuffers = nullptr;
   }

   if(_audioOutBuffers)
   {
      delete [] _audioOutBuffers;
      _audioOutBuffers = nullptr;
   }

   if(_controls)
   {
      delete [] _controls;
   }

   if(_controlsOut)
   {
      delete [] _controlsOut;
   }

//    if(_ppfeatures)
//    {
//       delete [] _ppfeatures;
//       _ppfeatures = nullptr;
//    }
// 
//    if(_features)
//    {
//       delete [] _features;
//       _features = nullptr;
//    }
}

void Lv2PluginI::lv2_FillFeatures(LV2_Feature *features, LV2_Feature **ppfeatures, LV2_Options_Option* options)
{
   uint32_t i;
   for(i = 0; i < SIZEOF_ARRAY(lv2Features); i++)
   {
      features [i] = _features [i];

      if(features [i].URI == nullptr)
      {
         break;
      }

      if(i == _fOptions)
      {
         features [i].data = options;
      }
      else if(i == _fInstanceAccess)
      {
         features [i].data = nullptr;
      }
      else if(i == _fExtUiHost)
      {
         features [i].data = &state->extHost;
      }
      else if(i == _fExtUiHostD)
      {
         features [i].data = &state->extHost;
      }
      else if(i == _fDataAccess)
      {
         features [i].data = &state->extData;
      }
      else if(i == _fWrkSchedule)
      {
         features [i].data = &state->wrkSched;
      }
      else if(i == _fUiResize)
      {
         features [i].data = &state->uiResize;
      }
      else if(i == _fPrgHost)
      {
         features [i].data = &state->prgHost;
      }
      else if(i == _fMakePath)
      {
         features [i].data = &state->makePath;
      }
      else if(i == _fMapPath)
      {
         features [i].data = &state->mapPath;
      }

//       _ppifeatures [i] = &_ifeatures [i];
      ppfeatures [i] = &features [i];
   }

//    _ppifeatures [i] = nullptr;
   ppfeatures [i] = nullptr;
}


//---------------------------------------------------------
//   initPluginInstance
//    return true on error
//---------------------------------------------------------

bool Lv2PluginI::initPluginInstance(Plugin* plug, int chans, 
                                 float sampleRate, unsigned int segmentSize,
                                 bool useDenormalBias, float denormalBias)
{
  _sampleRate = _dSampleRate = sampleRate;
  _segmentSize = _segSize = segmentSize;
  _channel = chans;
  if(plug == 0)
  {
    fprintf(stderr, "Lv2PluginI::initPluginInstance: zero plugin\n");
    return true;
  }
  _plugin = plug;

//   if (_plugin->incReferences(1)==0)
//     return true;

  
  _state = new LV2PluginWrapper_State;
//   _state->inst = nullptr;
  _state->widget = nullptr;
  _state->uiInst = nullptr;
  _state->plugInst = nullptr;
  _state->_ifeatures = new LV2_Feature[SIZEOF_ARRAY(lv2Features)];
  _state->_ppifeatures = new LV2_Feature *[SIZEOF_ARRAY(lv2Features) + 1];
  _state->sif = this;
  _state->synth = static_cast<Lv2Plugin*>(plug);

   //use Lv2Plugin features as template
// 
//   _features = new LV2_Feature[SIZEOF_ARRAY(lv2Features)];
//   _ppfeatures = new LV2_Feature *[SIZEOF_ARRAY(lv2Features) + 1];   
//    
//    //LV2Synth::lv2state_FillFeatures(_state);
//    //lv2state_FillFeatures(_state);
//    lv2_FillFeatures(ifeatures, ppifeatures, options);

  
   Lv2Plugin* lv2plug = static_cast<Lv2Plugin*>(plug);
   
   //prepare features and options arrays
   LV2_Options_Option _tmpl_options [] =
   {
      {LV2_OPTIONS_INSTANCE, 0, lv2plug->mapUrid(LV2_P_SAMPLE_RATE), sizeof(float), lv2plug->mapUrid(LV2_ATOM__Float), &_sampleRate},
      {LV2_OPTIONS_INSTANCE, 0, lv2plug->mapUrid(LV2_P_MIN_BLKLEN), sizeof(int32_t), lv2plug->mapUrid(LV2_ATOM__Int), &Lv2Plugin::minBlockSize}, //&_segSize},
      {LV2_OPTIONS_INSTANCE, 0, lv2plug->mapUrid(LV2_P_MAX_BLKLEN), sizeof(int32_t), lv2plug->mapUrid(LV2_ATOM__Int), &_segSize},
      {LV2_OPTIONS_INSTANCE, 0, lv2plug->mapUrid(LV2_P_NOM_BLKLEN), sizeof(int32_t), lv2plug->mapUrid(LV2_ATOM__Int), &_segSize},
      {LV2_OPTIONS_INSTANCE, 0, lv2plug->mapUrid(LV2_P_SEQ_SIZE), sizeof(int32_t), lv2plug->mapUrid(LV2_ATOM__Int), &_segSize},
      {LV2_OPTIONS_INSTANCE, 0, lv2plug->mapUrid(LV2_CORE__sampleRate), sizeof(double), lv2plug->mapUrid(LV2_ATOM__Double), &_dSampleRate},
      {LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, nullptr}

   };

   _options = new LV2_Options_Option[SIZEOF_ARRAY(_tmpl_options)]; // last option is NULLs

   for(uint32_t i = 0; i < SIZEOF_ARRAY(_tmpl_options); i++)
   {
      _options [i] = _tmpl_options [i];
   }

  
   if (_plugin->incReferences(1)==0)
   {
      delete [] _state->_ppifeatures;
      _state->_ppifeatures = nullptr;
      delete [] _state->_ifeatures;
      _state->_ifeatures = nullptr;
      return true;
   }

  
//    _state->handle = _handle = lilv_plugin_instantiate(_synth->_handle, (double)MusEGlobal::sampleRate, _state->_ppifeatures);
   //LilvInstance* inst = lilv_plugin_instantiate(_handle, sampleRate, (LV2_Feature**)data);
   //LilvInstance* inst = lilv_plugin_instantiate(_handle, sampleRate, _state->_ppifeatures);
   //LilvInstance* inst = static_cast<LilvInstance*>(plug->instantiate(sampleRate, _state));
   _state->handle = _handle = static_cast<LilvInstance*>(plug->instantiate(sampleRate, _state));

   if(_handle == nullptr)
   {
      _plugin->incReferences(-1);
      delete [] _state->_ppifeatures;
      _state->_ppifeatures = nullptr;
      delete [] _state->_ifeatures;
      _state->_ifeatures = nullptr;
      return true;
   }
//    if(inst == nullptr)
//    {
//       delete [] ppifeatures;
//       //ppifeatures = nullptr;
//       delete [] ifeatures;
//       //ifeatures = nullptr;
//       return NULL;
//    }

   //if(_state->handle)
   //{
     SP_DBG_LV2_2("Plugin instantiated", label().toLatin1().constData());
   //}
   
   //return false;
  
  
  
  
  
  
  
  
  
  QString inst("-" + QString::number(_plugin->instNo()));
  _name  = _plugin->name() + inst;
  _label = _plugin->label() + inst;

  const unsigned long ins = _plugin->inports();
  const unsigned long outs = _plugin->outports();
  if(outs)
  {
    _instances = _channel / outs;
    // Ask for one more instance for remainder if required.
    const int re = _channel % outs;
    if(re != 0)
      ++_instances;
    if(_instances < 1)
      _instances = 1;
  }
  else
  if(ins)
  {
    _instances = _channel / ins;
    // Ask for one more instance for remainder if required.
    const int re = _channel % ins;
    if(re != 0)
      ++_instances;
    if(_instances < 1)
      _instances = 1;
  }
  else
    _instances = 1;

  _handle = new LADSPA_Handle[_instances];
  for(int i = 0; i < _instances; ++i)
    _handle[i]=nullptr;

  for(int i = 0; i < _instances; ++i)
  {
    #ifdef PLUGIN_DEBUGIN
    fprintf(stderr, "Lv2PluginI::initPluginInstance instance:%d\n", i);
    #endif

    _handle[i] = _plugin->instantiate(_sampleRate);
    if(_handle[i] == nullptr)
      return true;
  }

  const unsigned long port_count = _plugin->portCount();

  _audioInPorts = 0;
  _audioOutPorts = 0;
  _controlPorts = 0;
  _controlOutPorts = 0;
  
  unsigned long port = 0;
  for (int i = 0; i < _instances; ++i) {
        for (unsigned long k = 0; k < port_count; ++k) {
              if (_plugin->isAudioIn(k)) {
                    if(port < (unsigned long)_channel)
                      ++_audioInPorts;
                    ++port;
                    }
              }
        }
  port = 0;
  for (int i = 0; i < _instances; ++i) {
        for (unsigned long k = 0; k < port_count; ++k) {
              if (_plugin->isAudioOut(k)) {
                    if(port < (unsigned long)_channel)
                      ++_audioOutPorts;
                    ++port;
                    }
              }
        }
        
  for(unsigned long k = 0; k < port_count; ++k)
  {
    if(_plugin->isParameterIn(k))
      ++_controlPorts;
    else
    if(_plugin->isParameterOut(k))
      ++_controlOutPorts;
  }

  if(_controlPorts)
    _controls = new Port[_controlPorts];
  if(_controlOutPorts)
  {
    _controlsOut = new Port[_controlOutPorts];
    _controlsOutDummy = new Port[_controlOutPorts];
  }

  for(unsigned long k = 0; k < _controlPorts; ++k)
  {
    // Set the parameter input's initial value to the default.
    const float val = _plugin->defaultValue(k);
    _controls[k]._val = val;
    // All instances' parameter inputs share the same controls. 
    // We don't have a mechanism to expose the other instances' inputs.
    for(int i = 0; i < _instances; ++i)
      _plugin->connectCtrlInport(_handle[i], k, &_controls[k]._val);
  }
  
  for(unsigned long k = 0; k < _controlOutPorts; ++k)
  {
    // Set the parameter output's initial value to zero.
    _controlsOut[k]._val = 0.0f;
    // Check for a latency port.
    const char* pname = _plugin->getParameterOutName(k);
    if(pname == QString("latency") || pname == QString("_latency"))
    {
      _hasLatencyOutPort = true;
      _latencyOutPort = k;
    }
    // Connect only the first instance's parameter output controls.
    // We don't have a mechanism to display the other instances' outputs.
    if(_instances > 0)
    {
      _plugin->connectCtrlOutport(_handle[0], k, &_controlsOut[k]._val);
      // Connect the rest to dummy ports.
      for(int i = 1; i < _instances; ++i)
        _plugin->connectCtrlOutport(_handle[i], k, &_controlsOutDummy[k]._val);
    }
  }

  int rv = posix_memalign((void **)&_audioInSilenceBuf, 16, sizeof(float) * _segmentSize);

  if(rv != 0)
  {
      fprintf(stderr, 
        "ERROR: Lv2PluginI::initPluginInstance: _audioInSilenceBuf posix_memalign returned error:%d. Aborting!\n", rv);
      abort();
  }

  if(useDenormalBias)
  {
      for(unsigned q = 0; q < _segmentSize; ++q)
      {
        _audioInSilenceBuf[q] = denormalBias;
      }
  }
  else
  {
      memset(_audioInSilenceBuf, 0, sizeof(float) * _segmentSize);
  }

  rv = posix_memalign((void **)&_audioOutDummyBuf, 16, sizeof(float) * _segmentSize);

  if(rv != 0)
  {
      fprintf(stderr, "ERROR: Lv2PluginI::initPluginInstance: _audioOutDummyBuf posix_memalign returned error:%d. Aborting!\n", rv);
      abort();
  }

  // Don't activate yet.
  //activate();
  return false;
}

     
} // namespace MusESimplePlugin
