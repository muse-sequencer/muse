//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id: osc.cpp,v 1.0.0.0 2010/04/22 03:39:58 terminator356 Exp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//  OSC module added by Tim.
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

#ifdef OSC_SUPPORT

// Turn on debugging messages
//#define OSC_DEBUG 

#include <string.h>
//#include <signal.h>
//#include <dlfcn.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
//#include <dssi.h>
//#include <alsa/asoundlib.h>

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QTimer>

#include <lo/lo.h>

#ifdef DSSI_SUPPORT
#include "dssihost.h"
#endif

#include "stringparam.h"
#include "plugin.h"
#include "track.h"
#include "song.h"
#include "synth.h"
//#include "audio.h"
//#include "jackaudio.h"
//#include "midi.h"
//#include "midiport.h"
//#include "al/al.h"
//#include "al/xml.h"
//#include "xml.h"
//#include "midictrl.h"
//#include "ladspaplugin.h"

#include "app.h"
#include "globals.h"
#include "globaldefs.h"
//#include "al/dsp.h"

static lo_server_thread serverThread = 0;
///static char osc_path_tmp[1024];
static char* url = 0;
static bool oscServerRunning = false;

//---------------------------------------------------------
//   oscError
//---------------------------------------------------------

static void oscError(int num, const char *msg, const char *path)
      {
      fprintf(stderr, "MusE: liblo server error %d in path %s: %s\n",
          num, path, msg);
      }

//---------------------------------------------------------
//   oscDebugHandler
//---------------------------------------------------------

static int oscDebugHandler(const char* path, const char* types, lo_arg** argv,
   int argc, void*, void*)
      {
      printf("MusE: got unhandled OSC message:\n   path: <%s>\n", path);
      for (int i = 0; i < argc; i++) {
            printf("   arg %d '%c' ", i, types[i]);
            lo_arg_pp(lo_type(types[i]), argv[i]);
            printf("\n");
            }
      return 1;
      }

//---------------------------------------------------------
//   oscMessageHandler
//---------------------------------------------------------

int oscMessageHandler(const char* path, const char* types, lo_arg** argv,
   int argc, void* data, void* user_data)
   //int argc, lo_message data, void* user_data)
{
  const char* p = path;
  
  // NOTE: Tried this, always returns 0 sec and 1 fractional. Shame, looks like timestamps are not used.
  //lo_timetag lo_tt = lo_message_get_timestamp(data);
  
  #ifdef OSC_DEBUG 
  if(argc) 
  {
      printf("oscMessageHandler: path:%s argc:%d\n", path, argc);
      for(int i = 0; i < argc; ++i) 
      {
        printf(" ");
        lo_arg_pp((lo_type)types[i], argv[i]);
      }
      printf("\n");
  } 
  else 
  {
      printf("%s\n", path);
      printf("oscMessageHandler: no args, path:%s\n", path);
  }
  #endif  
    
  bool isSynth = false;
  
  #ifdef DSSI_SUPPORT
  if(strncmp(p, "/dssi_synth/", 12) == 0)
  {
    isSynth = true;
    p += 12;
  }
  else
  #endif
  if(strncmp(p, "/ladspa_efx/", 12) == 0)
  {
    p += 12;
  }
  else
    return oscDebugHandler(path, types, argv, argc, data, user_data);

  TrackList* tl = song->tracks();
  

  #ifdef OSC_DEBUG 
  if(isSynth)
    fprintf(stderr, "oscMessageHandler: got message for dssi synth...\n");
  else  
    fprintf(stderr, "oscMessageHandler: got message for ladspa effect...\n");
  #endif
    
  // FIXME: Slowdowns: Shouldn't need these retries but they are needed, only upon creation of the synth. 
  // Need to fix the real source of the problem! The instance is taking too long to appear after creation.
  //
  ///for(int retry = 0; retry < 5000; ++retry) 
  {
    ///#ifdef OSC_DEBUG 
    ///fprintf(stderr, "oscMessageHandler: search retry number:%d ...\n", retry);
    ///#endif
    
    //if(_uiOscPath)
    //  break;
  
    #ifdef DSSI_SUPPORT
    if(isSynth)
    {
      // Message is meant for a dssi synth. Check dssi synth instances...
      SynthIList* sl = song->syntis();
      for(iSynthI si = sl->begin(); si != sl->end(); ++si) 
      {
        SynthI* synti = *si;
        
        #ifdef OSC_DEBUG 
        fprintf(stderr, "oscMessageHandler: searching for:%s checking synth instance:%s\n", p, synti->name().toLatin1().constData());
        #endif
        
	QByteArray ba = synti->name().toLatin1();
        const char* sub = strstr(p, ba.constData());
        if(sub == NULL) 
          continue;
        
        //DssiSynthIF* instance = (DssiSynthIF*)synti->sif();
        // TODO: Fix this dynamic cast - it may be a slowdown.
        DssiSynthIF* instance = dynamic_cast<DssiSynthIF*>(synti->sif());
        if(!instance)
          break;
          
        QByteArray ba2 = synti->name().toLatin1();
        p = sub + strlen(ba2.constData());
        
        if (*p != '/' || *(p + 1) == 0)
        {
          fprintf(stderr, "oscMessageHandler error: synth: end of path or no /\n");
          return oscDebugHandler(path, types, argv, argc, data, user_data);
        }
              
        ++p;
  
        #ifdef OSC_DEBUG 
        fprintf(stderr, "oscMessageHandler: synth track:%s method:%s\n", synti->name().toLatin1().constData(), p);
        #endif
        
        OscIF& oscif = instance->oscIF();
        
        if (!strcmp(p, "configure") && argc == 2 && !strcmp(types, "ss"))
              return oscif.oscConfigure(argv);
        else if (!strcmp(p, "control") && argc == 2 && !strcmp(types, "if"))
              return oscif.oscControl(argv);
        else if (!strcmp(p, "midi") && argc == 1 && !strcmp(types, "m"))
              return oscif.oscMidi(argv);
        else if (!strcmp(p, "program") && argc == 2 && !strcmp(types, "ii"))
              return oscif.oscProgram(argv);
        else if (!strcmp(p, "update") && argc == 1 && !strcmp(types, "s"))
              return oscif.oscUpdate(argv);
        else if (!strcmp(p, "exiting") && argc == 0)
              return oscif.oscExiting(argv);
        return oscDebugHandler(path, types, argv, argc, data, user_data);
      }
    }
    else
    #endif //DSSI_SUPPORT
    // Message is meant for a ladspa effect. Check all ladspa effect instances...
    for(ciTrack it = tl->begin(); it != tl->end(); ++it) 
    {
      if((*it)->isMidiTrack())
        continue;
        
      Pipeline* efxPipe = ((AudioTrack*)*it)->efxPipe();
      if(efxPipe)
      {
        for(ciPluginI ip = efxPipe->begin(); ip != efxPipe->end(); ++ip)
        {
          PluginI* instance = *ip;
          if(!instance)
            continue;
          
          #ifdef OSC_DEBUG 
          fprintf(stderr, "oscMessageHandler: searching for:%s checking effect instance:%s label:%s lib:%s\n", 
                  p, instance->name().toLatin1().constData(), instance->label().toLatin1().constData(), instance->lib().toLatin1().constData());
          #endif
          
          //const char* sub = strstr(p, instance->name().toLatin1().constData());
          ///const char* sub = strstr(p, instance->label().toLatin1().constData());
          QByteArray ba = instance->label().toLatin1();
          const char* sub = strstr(p, ba.constData());
          if(sub == NULL) 
            continue;
            
          Plugin* plugin = instance->plugin();
          if(!plugin)
            break;
          
          //p = sub + strlen(instance->name().toLatin1().constData());
          QByteArray ba3 = instance->label().toLatin1();
          p = sub + strlen(ba3.constData());
          
          if (*p != '/' || *(p + 1) == 0)
          {
            fprintf(stderr, "oscMessageHandler: error: effect: end of path or no /\n");
            return oscDebugHandler(path, types, argv, argc, data, user_data);
          }
                
          ++p;
    
          #ifdef OSC_DEBUG 
          //fprintf(stderr, "oscMessageHandler: effect:%s method:%s\n", instance->name().toLatin1().constData(), p);
          fprintf(stderr, "oscMessageHandler: effect:%s method:%s\n", instance->label().toLatin1().constData(), p);
          #endif
          
          OscIF& oscif = instance->oscIF();
          
          if (!strcmp(p, "configure") && argc == 2 && !strcmp(types, "ss"))
                return oscif.oscConfigure(argv);
          else if (!strcmp(p, "control") && argc == 2 && !strcmp(types, "if"))
                return oscif.oscControl(argv);
          else if (!strcmp(p, "midi") && argc == 1 && !strcmp(types, "m"))
                return oscif.oscMidi(argv);
          else if (!strcmp(p, "program") && argc == 2 && !strcmp(types, "ii"))
                return oscif.oscProgram(argv);
          else if (!strcmp(p, "update") && argc == 1 && !strcmp(types, "s"))
                return oscif.oscUpdate(argv);
          else if (!strcmp(p, "exiting") && argc == 0)
                return oscif.oscExiting(argv);
          return oscDebugHandler(path, types, argv, argc, data, user_data);
        }
      }
    }
      
    ///usleep(1000);
  }
  
  fprintf(stderr, "oscMessageHandler: timeout error: no synth or effect instance found for given path\n");
  return oscDebugHandler(path, types, argv, argc, data, user_data);
}


//---------------------------------------------------------
//   initOSC
//---------------------------------------------------------

void initOSC()
{
  if(url)
    free(url);
  url = 0;
    
  // Create OSC thread
  // Only if not created yet. 
  if(!serverThread)
  {
    serverThread = lo_server_thread_new(0, oscError);
    if(!serverThread)
    {
      printf("initOSC() Failed to create OSC server!\n");
      return;
    }
  }  
  
  ///snprintf(osc_path_tmp, 31, "/dssi");
  // Test: Clear the temp path:
  //snprintf(osc_path_tmp, 31, "");
  
  ///char* tmp = lo_server_thread_get_url(serverThread);
  
  url = lo_server_thread_get_url(serverThread);
  if(!url)
  {
    lo_server_thread_free(serverThread);
    printf("initOSC() Failed to get OSC server thread url !\n");
    return;
  }
  
  ///url = (char *)malloc(strlen(tmp) + strlen(osc_path_tmp));
  //url = (char *)malloc(strlen(tmp));
  
  ///sprintf(url, "%s%s", tmp, osc_path_tmp + 1);
  //sprintf(url, "%s", tmp, osc_path_tmp + 1);
  
  ///free(tmp);
  
  lo_method meth = 0;
  meth = lo_server_thread_add_method(serverThread, 0, 0, oscMessageHandler, 0);
  if(!meth)
  {
    printf("initOSC() Failed to add oscMessageHandler method to OSC server!\n");
    // Does not return a value.
    lo_server_thread_free(serverThread);
    serverThread = 0;
    free(url);
    url = 0;
    return;
  }
  
  // Does not return a value.
  lo_server_thread_start(serverThread);
}

//---------------------------------------------------------
//   exitOSC
//---------------------------------------------------------

void exitOSC()
{
  oscServerRunning = false;  
  if(serverThread)
  {
    // Does not return a value.
    lo_server_thread_stop(serverThread);
    lo_server_thread_free(serverThread);
  }  
  serverThread = 0;
}

//---------------------------------------------------------
//   startOSC
//---------------------------------------------------------

void startOSC()
{
  if(serverThread)
    // Does not return a value.
    lo_server_thread_start(serverThread);
  oscServerRunning = true;  
}

//---------------------------------------------------------
//   stopOSC
//---------------------------------------------------------

void stopOSC()
{
  if(serverThread)
    // Does not return a value.
    lo_server_thread_stop(serverThread);
  oscServerRunning = false;  
}
        


/*
//---------------------------------------------------------
//   OscControlFifo
//    put
//    return true on fifo overflow
//---------------------------------------------------------

bool OscControlFifo::put(const OscControlValue& event)
      {
      if (size < OSC_FIFO_SIZE) {
            fifo[wIndex] = event;
            wIndex = (wIndex + 1) % OSC_FIFO_SIZE;
            // q_atomic_increment(&size);
            ++size;
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   get
//---------------------------------------------------------

OscControlValue OscControlFifo::get()
      {
      OscControlValue event(fifo[rIndex]);
      rIndex = (rIndex + 1) % OSC_FIFO_SIZE;
      // q_atomic_decrement(&size);
      --size;
      return event;
      }

//---------------------------------------------------------
//   peek
//---------------------------------------------------------

const OscControlValue& OscControlFifo::peek(int n)
      {
      int idx = (rIndex + n) % OSC_FIFO_SIZE;
      return fifo[idx];
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void OscControlFifo::remove()
      {
      rIndex = (rIndex + 1) % OSC_FIFO_SIZE;
      // q_atomic_decrement(&size);
      --size;
      }
*/


//---------------------------------------------------------
//   OscIF
//   Open Sound Control Interface
//---------------------------------------------------------

OscIF::OscIF()
{
  //_oscPluginI = 0;
  
  //#ifdef DSSI_SUPPORT
  //_oscSynthIF = 0;
  //#endif
  
  _uiOscTarget = 0;
  _uiOscSampleRatePath = 0;
  _uiOscShowPath = 0;
  _uiOscControlPath = 0;
  _uiOscConfigurePath = 0;
  _uiOscProgramPath = 0;
  _uiOscPath = 0;
  //guiPid = -1;
  _oscGuiQProc = 0;
  _oscGuiVisible = false;
  
  //_oscControlFifos = 0;
}

OscIF::~OscIF()
{
  //if (guiPid != -1)
  //      kill(guiPid, SIGHUP);
  if(_oscGuiQProc)
  {
    if(_oscGuiQProc->state())
    {
      #ifdef OSC_DEBUG 
      printf("OscIF::~OscIF terminating _oscGuiQProc\n");
      #endif
      
      //_oscGuiQProc->kill();
      // "This tries to terminate the process the nice way. If the process is still running after 5 seconds, 
      //  it terminates the process the hard way. The timeout should be chosen depending on the time the 
      //  process needs to do all its cleanup: use a higher value if the process is likely to do a lot of 
      //  computation or I/O on cleanup."           
      _oscGuiQProc->terminate();
      // FIXME: In Qt4 this can only be used with threads started with QThread. 
      // Kill is bad anyway, app should check at close if all these guis closed or not 
      //  and ask user if they really want to close, possibly with kill.
      // Terminate might not terminate the thread. It is given a chance to prompt for saving etc.
      //  so kill is not desirable.
      // We could wait until terminate finished but don't think that's good here.
      ///QTimer::singleShot( 5000, _oscGuiQProc, SLOT( kill() ) );          
    }  
    //delete _oscGuiQProc;
  }
      
  if(_uiOscTarget)
    lo_address_free(_uiOscTarget);
  if(_uiOscSampleRatePath)
    free(_uiOscSampleRatePath);
  if(_uiOscShowPath)
    free(_uiOscShowPath);
  if(_uiOscControlPath)
    free(_uiOscControlPath);
  if(_uiOscConfigurePath)
    free(_uiOscConfigurePath);
  if(_uiOscProgramPath)
    free(_uiOscProgramPath);
  if(_uiOscPath)
    free(_uiOscPath);
    
  //if(_oscControlFifos)
  //  delete[] _oscControlFifos;
}

/*
//---------------------------------------------------------
//   oscFifo
//---------------------------------------------------------

OscControlFifo* OscIF::oscFifo(unsigned long i) const
{
  if(!_oscControlFifos)
    return 0;
  return &_oscControlFifos[i];
}
*/

//---------------------------------------------------------
//   oscUpdate
//---------------------------------------------------------

int OscIF::oscUpdate(lo_arg **argv)
{
      const char *purl = (char *)&argv[0]->s;

      if(_uiOscTarget)
        lo_address_free(_uiOscTarget);
      _uiOscTarget = 0;  
      char* host   = lo_url_get_hostname(purl);
      char* port   = lo_url_get_port(purl);
      _uiOscTarget = lo_address_new(host, port);
      free(host);
      free(port);
      if(!_uiOscTarget)
        return 0;
        
      if (_uiOscPath)
            free(_uiOscPath);
      _uiOscPath = lo_url_get_path(purl);
      int pl = strlen(_uiOscPath);

      if(_uiOscSampleRatePath)
        free(_uiOscSampleRatePath);
      _uiOscSampleRatePath = (char *)malloc(pl + 14);
      sprintf(_uiOscSampleRatePath, "%s/sample-rate", _uiOscPath);
        
      if (_uiOscControlPath)
            free(_uiOscControlPath);
      _uiOscControlPath = (char *)malloc(pl + 10);
      sprintf(_uiOscControlPath, "%s/control", _uiOscPath);

      if (_uiOscConfigurePath)
            free(_uiOscConfigurePath);
      _uiOscConfigurePath = (char *)malloc(pl + 12);
      sprintf(_uiOscConfigurePath, "%s/configure", _uiOscPath);

      if (_uiOscProgramPath)
            free(_uiOscProgramPath);
      _uiOscProgramPath = (char *)malloc(pl + 10);
      sprintf(_uiOscProgramPath, "%s/program", _uiOscPath);

      if (_uiOscShowPath)
            free(_uiOscShowPath);
      _uiOscShowPath = (char *)malloc(pl + 10);
      sprintf(_uiOscShowPath, "%s/show", _uiOscPath);

      /* At this point a more substantial host might also call
      * configure() on the UI to set any state that it had remembered
      * for the plugin instance.  But we don't remember state for
      * plugin instances (see our own configure() implementation in
      * osc_configure_handler), and so we have nothing to send except
      * the optional project directory.
      */

      #ifdef OSC_DEBUG 
      printf("OscIF::oscUpdate: _uiOscTarget:%p\n", _uiOscTarget);
      if(url)
        printf(" server url:%s\n", url);
      else  
        printf(" no server url!\n");
      printf(" update path:%s\n", purl);
      printf(" _uiOscPath:%s\n", _uiOscPath);
      printf(" _uiOscSampleRatePath:%s\n", _uiOscSampleRatePath);
      printf(" _uiOscConfigurePath:%s\n", _uiOscConfigurePath);
      printf(" _uiOscProgramPath:%s\n", _uiOscProgramPath);
      printf(" _uiOscControlPath:%s\n",_uiOscControlPath);
      printf(" _uiOscShowPath:%s\n", _uiOscShowPath);
      printf(" museProject:%s\n", museProject.toLatin1().constData());
      #endif
      
      // Send sample rate.
      lo_send(_uiOscTarget, _uiOscSampleRatePath, "i", sampleRate);
      
      // Send project directory.
      //lo_send(_uiOscTarget, _uiOscConfigurePath, "ss",
      //   DSSI_PROJECT_DIRECTORY_KEY, museProject.toLatin1().constData());  // song->projectPath()
      
      // Done in sub-classes.
      /*
      #ifdef DSSI_SUPPORT
      //lo_send(_uiOscTarget, _uiOscConfigurePath, "ss",
         //DSSI_PROJECT_DIRECTORY_KEY, song->projectPath().toAscii().data());
      lo_send(_uiOscTarget, _uiOscConfigurePath, "ss",
         DSSI_PROJECT_DIRECTORY_KEY, museProject.toLatin1().constData());
      
      if(_oscSynthIF)
      {
        for(ciStringParamMap r = _oscSynthIF->synthI->_stringParamMap.begin(); r != synti->_stringParamMap.end(); ++r) 
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
      #endif
      */
      
      /*
      char uiOscGuiPath[strlen(_uiOscPath)+6];
      sprintf(uiOscGuiPath, "%s/%s", _uiOscPath, "show");
      
      #ifdef OSC_DEBUG 
      printf("OscIF::oscUpdate Sending show uiOscGuiPath:%s\n", uiOscGuiPath);
      #endif
      
      lo_send(_uiOscTarget, uiOscGuiPath, "");
      
      sprintf(uiOscGuiPath, "%s/%s", _uiOscPath, "hide");
      
      #ifdef OSC_DEBUG 
      printf("OscIF::oscUpdate Sending hide uiOscGuiPath:%s\n", uiOscGuiPath);
      #endif
      
      lo_send(_uiOscTarget, uiOscGuiPath, "");
      */
      
#if 0
      /* Send current bank/program  (-FIX- another race...) */
      if (instance->pendingProgramChange < 0) {
            unsigned long bank = instance->currentBank;
            unsigned long program = instance->currentProgram;
            instance->uiNeedsProgramUpdate = 0;
            if (instance->uiTarget) {
                  lo_send(instance->uiTarget, instance->ui_osc_program_path, "ii", bank, program);
                  }
            }

      /* Send control ports */
      for (i = 0; i < instance->plugin->controlIns; i++) {
            int in = i + instance->firstControlIn;
            int port = pluginControlInPortNumbers[in];
            lo_send(instance->uiTarget, instance->ui_osc_control_path, "if", port,
               pluginControlIns[in]);
            /* Avoid overloading the GUI if there are lots and lots of ports */
            if ((i+1) % 50 == 0)
                  usleep(300000);
            }
#endif
      return 0;
}

//---------------------------------------------------------
//   oscExiting
//---------------------------------------------------------

int OscIF::oscExiting(lo_arg**)
{
      // The gui is gone now, right?
      _oscGuiVisible = false;
      
      if(_oscGuiQProc)
      {
        if(_oscGuiQProc->state())
        {
          #ifdef OSC_DEBUG 
          printf("OscIF::oscExiting terminating _oscGuiQProc\n");
          #endif
          
          //_oscGuiQProc->kill();
          // "This tries to terminate the process the nice way. If the process is still running after 5 seconds, 
          //  it terminates the process the hard way. The timeout should be chosen depending on the time the 
          //  process needs to do all its cleanup: use a higher value if the process is likely to do a lot of 
          //  computation or I/O on cleanup."           
          _oscGuiQProc->terminate();
          // FIXME: In Qt4 this can only be used with threads started with QThread. 
          // Kill is bad anyway, app should check at close if all these guis closed or not 
          //  and ask user if they really want to close, possibly with kill.
          // Terminate might not terminate the thread. It is given a chance to prompt for saving etc.
          //  so kill is not desirable.
          // We could wait until terminate finished but don't think that's good here.
          ///QTimer::singleShot( 5000, _oscGuiQProc, SLOT( kill() ) );          
        }  
        //delete _oscGuiQProc;
      }
          
      if(_uiOscTarget)
        lo_address_free(_uiOscTarget);
      _uiOscTarget = 0;  
      if(_uiOscSampleRatePath)
        free(_uiOscSampleRatePath);
      _uiOscSampleRatePath = 0;  
      if(_uiOscShowPath)
        free(_uiOscShowPath);
      _uiOscShowPath = 0;  
      if(_uiOscControlPath)
        free(_uiOscControlPath);
      _uiOscControlPath = 0;  
      if(_uiOscConfigurePath)
        free(_uiOscConfigurePath);
      _uiOscConfigurePath = 0;  
      if(_uiOscProgramPath)
        free(_uiOscProgramPath);
      _uiOscProgramPath = 0;  
      if(_uiOscPath)
        free(_uiOscPath);
      _uiOscPath = 0;  
        
      //if(_oscControlFifos)
      //  delete[] _oscControlFifos;
      
      //const DSSI_Descriptor* dssi = synth->dssi;
      //const LADSPA_Descriptor* ld = dssi->LADSPA_Plugin;
      //if(ld->deactivate) 
      //  ld->deactivate(handle);
      
      /*
      if (_uiOscPath == 0) {
            printf("OscIF::oscExiting(): no _uiOscPath\n");
            return 1;
            }
      char uiOscGuiPath[strlen(_uiOscPath)+6];
        
      sprintf(uiOscGuiPath, "%s/%s", _uiOscPath, "quit");
      #ifdef OSC_DEBUG 
      printf("OscIF::oscExiting(): sending quit to uiOscGuiPath:%s\n", uiOscGuiPath);
      #endif
      
      lo_send(_uiOscTarget, uiOscGuiPath, "");
      */
      
#if 0
      int i;

      if (verbose) {
            printf("MusE: OSC: got exiting notification for instance %d\n",
               instance->number);
            }

      if (instance->plugin) {

            /*!!! No, this isn't safe -- plugins deactivated in this way
              would still be included in a run_multiple_synths call unless
              we re-jigged the instance array at the same time -- leave it
              for now
            if (instance->plugin->descriptor->LADSPA_Plugin->deactivate) {
                  instance->plugin->descriptor->LADSPA_Plugin->deactivate
                     (instanceHandles[instance->number]);
                  }
            */
            /* Leave this flag though, as we need it to determine when to exit */
            instance->inactive = 1;
            }

      /* Do we have any plugins left running? */

      for (i = 0; i < instance_count; ++i) {
            if (!instances[i].inactive)
                  return 0;
            }

      if (verbose) {
            printf("MusE: That was the last remaining plugin, exiting...\n");
            }
      exiting = 1;
#endif
      return 0;
}

//---------------------------------------------------------
//   oscSendProgram
//---------------------------------------------------------

void OscIF::oscSendProgram(unsigned long prog, unsigned long bank)
{
  if(_uiOscTarget && _uiOscProgramPath)
    lo_send(_uiOscTarget, _uiOscProgramPath, "ii", bank, prog);
}

//---------------------------------------------------------
//   oscSendControl
//---------------------------------------------------------

void OscIF::oscSendControl(unsigned long dssiPort, float v)
{
  if(_uiOscTarget && _uiOscControlPath)
    lo_send(_uiOscTarget, _uiOscControlPath, "if", dssiPort, v);
}

//---------------------------------------------------------
//   oscSendConfigure
//---------------------------------------------------------

void OscIF::oscSendConfigure(const char *key, const char *val)
{ 
  if(_uiOscTarget && _uiOscConfigurePath)
    lo_send(_uiOscTarget, _uiOscConfigurePath, "ss", key, val);
}

//---------------------------------------------------------
//   oscInitGui
//---------------------------------------------------------

//bool OscIF::oscInitGui()
bool OscIF::oscInitGui(const QString& typ, const QString& baseName, const QString& name, 
                       const QString& label, const QString& filePath, const QString& guiPath)
{
      // Are we already running? We don't want to allow another process do we...
      if((_oscGuiQProc != 0) && (_oscGuiQProc->state()))
        return true;
        
      if(!url)
      {  
        fprintf(stderr, "OscIF::oscInitGui no server url!\n");
        return false;
      }
            
      if(guiPath.isEmpty())
      {  
        fprintf(stderr, "OscIF::oscInitGui guiPath is empty\n");
        return false;
      }
            
      //
      //  start gui
      //
      //static char oscUrl[1024];
      //char oscUrl[1024];
      QString oscUrl;
      
      //snprintf(oscUrl, 1024, "%s/%s/%s", url, baseName.ascii(), name.ascii());
      //snprintf(oscUrl, 1024, "%s%s/%s/%s", url, typ.toLatin1().constData(), baseName.toLatin1().constData(), name.toLatin1().constData());
      //oscUrl = QString("%1%2/%3/%4").arg(QString(QT_TRANSLATE_NOOP("@default", url))).arg(typ).arg(baseName).arg(name);
      oscUrl = QString("%1%2/%3/%4").arg(QString(QT_TRANSLATE_NOOP("@default", url))).arg(typ).arg(baseName).arg(label);
      
      // Removed p4.0.19 Tim
      /*
      //QString guiPath(info.path() + "/" + info.baseName());
      //QString guiPath(synth->info.dirPath() + "/" + synth->info.baseName());
      QString guiPath(dirPath + "/" + baseName);

      #ifdef OSC_DEBUG 
      fprintf(stderr, "OscIF::oscInitGui guiPath:%s\n", guiPath.toLatin1().constData());
      #endif
      
      QDir guiDir(guiPath, "*", QDir::Unsorted, QDir::Files);
      if (guiDir.exists()) 
      {
            //const QFileInfoList list = guiDir.entryInfoList();
            QStringList list = guiDir.entryList();
            
            //for (int i = 0; i < list.size(); ++i) {
            for (int i = 0; i < list.count(); ++i) 
            {
                
                //QFileInfo fi = list.at(i);
                QFileInfo fi(guiPath + QString("/") + list[i]);
                  
                  QString gui(fi.filePath());
                  if (gui.contains('_') == 0)
                        continue;
                  struct stat buf;
                  
                  //if (stat(gui.toAscii().data(), &buf)) {
                  if (stat(gui.toLatin1().constData(), &buf)) {
                  
                        perror("stat failed");
                        continue;
                        }

                  #ifdef OSC_DEBUG 
                  fprintf(stderr, "OscIF::oscInitGui  %s %s %s %s\n",
                      //fi.filePath().toAscii().data(),
                      //fi.fileName().toAscii().data(),
                      fi.filePath().toLatin1().constData(),
                      //fi.fileName().ascii(),
                      
                      oscUrl.toLatin1().constData(),
                      
                      //synth->info.filePath().ascii(),
                      filePath.toLatin1().constData(),
                      
                      //name().toAscii().data(),
                      //synth->name().ascii());
                      name.toLatin1().constData());
                  #endif
                  */      
                      
                  //if ((S_ISREG(buf.st_mode) || S_ISLNK(buf.st_mode)) &&
                  //   (buf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) 
                  //{
                        
                        // Changed by T356.
                        // fork + execlp were causing the processes to remain after closing gui, requiring manual kill.
                        // Changed to QProcess, works OK now. 
                        //if((guiPid = fork()) == 0) 
                        //{
                              // No QProcess created yet? Do it now. Only once per SynthIF instance. Exists until parent destroyed.
                              if(_oscGuiQProc == 0)
                                //_oscGuiQProc = new QProcess(muse);                        
                                _oscGuiQProc = new QProcess();                        
			      
                              //QString program(fi.filePath());
                              QString program(guiPath);
			      QStringList arguments;
			      arguments << oscUrl
					<< filePath
					<< name
					<< QString("channel-1");

                              /*
                              fprintf(stderr, "OscIF::oscInitGui  %s %s %s %s\n",
                                //fi.filePath().toAscii().data(),
                                //fi.fileName().toAscii().data(),
                                guiPath.toLatin1().constData(),
                                //fi.fileName().ascii(),
                                
                                oscUrl.toLatin1().constData(),
                                
                                //synth->info.filePath().ascii(),
                                filePath.toLatin1().constData(),
                                
                                //name().toAscii().data(),
                                //synth->name().ascii());
                                name.toLatin1().constData());
                              */  
                      
			      /* Leave out Qt3 stuff for reference - Orcan:
                              // Don't forget this, he he...
                              _oscGuiQProc->clearArguments();
                              
                              _oscGuiQProc->addArgument(fi.filePath());
                              //_oscGuiQProc->addArgument(fi.fileName()); // No conventional 'Arg0' here.
                              //_oscGuiQProc->addArgument(QString(oscUrl));
                              _oscGuiQProc->addArgument(oscUrl);
                              //_oscGuiQProc->addArgument(synth->info.filePath());
                              _oscGuiQProc->addArgument(filePath);
                              //_oscGuiQProc->addArgument(synth->name());
                              _oscGuiQProc->addArgument(name);
                              _oscGuiQProc->addArgument(QString("channel-1"));
                              */
                              #ifdef OSC_DEBUG 
                              fprintf(stderr, "OscIF::oscInitGui starting QProcess\n");
                              #endif
			      _oscGuiQProc->start(program, arguments);
			      
                                
                              if(_oscGuiQProc->state())
                              {
                                #ifdef OSC_DEBUG 
                                fprintf(stderr, "OscIF::oscInitGui started QProcess\n");
                                #endif
                                
                                //guiPid = _oscGuiQProc->processIdentifier();
                              }
                              else
                              {
                              
                                /*
                                execlp(
                                        //fi.filePath().toAscii().data(),
                                        //fi.fileName().toAscii().data(),
                                        fi.filePath().ascii(),
                                        fi.fileName().ascii(),
                                        
                                        oscUrl,
                                        
                                        //info.filePath().toAscii().data(),
                                        //name().toAscii().data(),
                                        synth->info.filePath().ascii(),
                                        synth->name().ascii(),
                                        
                                        "channel 1", (void*)0);
                                */
                                        
                                fprintf(stderr, "exec %s %s %s failed: %s\n",
                                        //fi.filePath().toAscii().data(),
                                        //fi.fileName().toAscii().data(),
                                        //fi.filePath().toLatin1().constData(),
                                        guiPath.toLatin1().constData(),
                                        //fi.fileName().toLatin1().constData(),
                                        
                                        oscUrl.toLatin1().constData(),
                                        
                                        //name().toAscii().data(),
                                        //synth->name().ascii(),
                                        name.toLatin1().constData(),
                                        
                                        strerror(errno));
                                        
                                // It's Ok, Keep going. So nothing happens. So what. The timeout in showGui will just leave.
                                // Maybe it's a 'busy' issue somewhere - allow to try again later + save work now.
                                //exit(1);
                                
                              }
                              
                              #ifdef OSC_DEBUG 
                              fprintf(stderr, "OscIF::oscInitGui after QProcess\n");
                              #endif
                        //}
                  //}
            //}
            //synth->_hasGui = true;
      /*
      }
      else {
            printf("OscIF::oscInitGui %s: no dir for gui found: %s\n",
               //name().toAscii().data(), guiPath.toAscii().data());
               //synth->name().ascii(), guiPath.ascii());
               name.toLatin1().constData(), guiPath.toLatin1().constData());
            
            //synth->_hasGui = false;
            }
     */       
            
  return true;          
}

 
//---------------------------------------------------------
//   oscShowGui
//---------------------------------------------------------

void OscIF::oscShowGui(bool v)
{
      #ifdef OSC_DEBUG 
      printf("OscIF::oscShowGui(): v:%d visible:%d\n", v, oscGuiVisible());
      #endif
      
      if (v == oscGuiVisible())
            return;
      
      //if(guiPid == -1)
      if((_oscGuiQProc == 0) || (!_oscGuiQProc->state()))
      {
        // We need an indicator that update was called - update must have been called to get new path etc...
        // If the process is not running this path is invalid, right?
        if(_uiOscPath)
          free(_uiOscPath);
        _uiOscPath = 0;  
          
        #ifdef OSC_DEBUG
        printf("OscIF::oscShowGui(): No QProcess or process not running. Starting gui...\n");
        #endif
        
        if(!oscInitGui())
        {
          printf("OscIF::oscShowGui(): failed to initialize gui on oscInitGui()\n");
          return;
        }  
      }  
      
      //for (int i = 0; i < 5; ++i) {
      for (int i = 0; i < 10; ++i) {    // Give it a wee bit more time?
            if (_uiOscPath)
                  break;
            sleep(1);
            }
      if (_uiOscPath == 0) {
            printf("OscIF::oscShowGui(): no _uiOscPath. Error: Timeout - synth gui did not start within 10 seconds.\n");
            return;
            }
      
      char uiOscGuiPath[strlen(_uiOscPath)+6];
      sprintf(uiOscGuiPath, "%s/%s", _uiOscPath, v ? "show" : "hide");
      
      #ifdef OSC_DEBUG 
      printf("OscIF::oscShowGui(): Sending show/hide uiOscGuiPath:%s\n", uiOscGuiPath);
      #endif
      
      lo_send(_uiOscTarget, uiOscGuiPath, "");
      _oscGuiVisible = v;
}

//---------------------------------------------------------
//   oscGuiVisible
//---------------------------------------------------------

bool OscIF::oscGuiVisible() const
{
  return _oscGuiVisible;
}

#ifdef DSSI_SUPPORT

//---------------------------------------------------------
//   OscDssiIF::
//   oscSetSynthIF
//---------------------------------------------------------

//void OscIF::oscSetSynthIF(DssiSynthIF* s) 
void OscDssiIF::oscSetSynthIF(DssiSynthIF* s)
{ 
  _oscSynthIF = s;
  //if(_oscControlFifos)
  //  delete[] _oscControlFifos;
  //_oscControlFifos = 0;
    
  //if(_oscSynthIF && _oscSynthIF->dssiSynth())
  //{
  //  unsigned long ports = _oscSynthIF->dssiSynth()->inControls();
  //  _oscControlFifos = new OscControlFifo[ports];  
  //}  
}

//---------------------------------------------------------
//   oscUpdate
//---------------------------------------------------------

int OscDssiIF::oscUpdate(lo_arg **argv)
{
      // Make sure to call base method.
      OscIF::oscUpdate(argv);
      
      // Send sample rate. No, done in base class.
      //lo_send(_uiOscTarget, _uiOscSampleRatePath, "i", sampleRate);
      
      // Send project directory. No, done in DssiSynthIF.
      //lo_send(_uiOscTarget, _uiOscConfigurePath, "ss",
      //   DSSI_PROJECT_DIRECTORY_KEY, museProject.toLatin1().constData());  // song->projectPath()
      
      if(_oscSynthIF)
        _oscSynthIF->oscUpdate();
      
      /*
      if(_oscSynthIF)
      {
        // Send current string configuration parameters.
        StringParamMap& map = _oscSynthIF->dssiSynthI()->stringParameters();
        int i = 0;
        for(ciStringParamMap r = map.begin(); r != map.end(); ++r) 
        {
          lo_send(_uiOscTarget, _uiOscConfigurePath, "ss", r->first.c_str(), r->second.c_str());
          // Avoid overloading the GUI if there are lots and lots of params. 
          if((i+1) % 50 == 0)
            usleep(300000);
          ++i;      
        }  
        
        // Send current bank and program.
        unsigned long bank, prog;
        _oscSynthIF->dssiSynthI()->currentProg(&prog, &bank, 0);
        lo_send(_uiOscTarget, _uiOscProgramPath, "ii", bank, prog);
        
        // Send current control values.
        unsigned long ports = _oscSynthIF->dssiSynth()->inControls();
        for(unsigned long i = 0; i < ports; ++i) 
        {
          unsigned long k = _oscSynthIF->dssiSynth()->inControlPortIdx(i);
          lo_send(_uiOscTarget, _uiOscControlPath, "if", k, _oscSynthIF->getParameter(i));
          // Avoid overloading the GUI if there are lots and lots of ports. 
          if((i+1) % 50 == 0)
            usleep(300000);
        }
      }  
      */
      
      /*
      char uiOscGuiPath[strlen(_uiOscPath)+6];
      sprintf(uiOscGuiPath, "%s/%s", _uiOscPath, "show");
      
      #ifdef OSC_DEBUG 
      printf("OscIF::oscUpdate Sending show uiOscGuiPath:%s\n", uiOscGuiPath);
      #endif
      
      lo_send(_uiOscTarget, uiOscGuiPath, "");
      
      sprintf(uiOscGuiPath, "%s/%s", _uiOscPath, "hide");
      
      #ifdef OSC_DEBUG 
      printf("OscIF::oscUpdate Sending hide uiOscGuiPath:%s\n", uiOscGuiPath);
      #endif
      
      lo_send(_uiOscTarget, uiOscGuiPath, "");
      */
      
#if 0
      /* Send current bank/program  (-FIX- another race...) */
      if (instance->pendingProgramChange < 0) {
            unsigned long bank = instance->currentBank;
            unsigned long program = instance->currentProgram;
            instance->uiNeedsProgramUpdate = 0;
            if (instance->uiTarget) {
                  lo_send(instance->uiTarget, instance->ui_osc_program_path, "ii", bank, program);
                  }
            }

      /* Send control ports */
      for (i = 0; i < instance->plugin->controlIns; i++) {
            int in = i + instance->firstControlIn;
            int port = pluginControlInPortNumbers[in];
            lo_send(instance->uiTarget, instance->ui_osc_control_path, "if", port,
               pluginControlIns[in]);
            /* Avoid overloading the GUI if there are lots and lots of ports */
            if ((i+1) % 50 == 0)
                  usleep(300000);
            }
#endif
      return 0;
}


//---------------------------------------------------------
//   oscConfigure
//---------------------------------------------------------

int OscDssiIF::oscConfigure(lo_arg** argv)
{
  //OscIF::oscConfigure(argv);
  
  if(_oscSynthIF)
    _oscSynthIF->oscConfigure((const char*)&argv[0]->s, (const char*)&argv[1]->s);
  return 0;
}

//---------------------------------------------------------
//   oscMidi
//---------------------------------------------------------

int OscDssiIF::oscMidi(lo_arg** argv)
{
  //OscIF::oscMidi(argv);
  
  if(_oscSynthIF)
    _oscSynthIF->oscMidi(argv[0]->m[1], argv[0]->m[2], argv[0]->m[3]);
  
  return 0;
}

//---------------------------------------------------------
//   oscProgram
//---------------------------------------------------------

int OscDssiIF::oscProgram(lo_arg** argv)
{
  //OscIF::oscProgram(argv);
  
  if(_oscSynthIF)
    _oscSynthIF->oscProgram(argv[1]->i, argv[0]->i);
  
  return 0;
}

//---------------------------------------------------------
//   oscControl
//---------------------------------------------------------

int OscDssiIF::oscControl(lo_arg** argv)
{
  //OscIF::oscControl(argv);
  
  int port = argv[0]->i;
  if(port < 0)
    return 0;
    
  if(_oscSynthIF)
    _oscSynthIF->oscControl(argv[0]->i, argv[1]->f);
  
  return 0;
}

//---------------------------------------------------------
//   oscInitGui
//---------------------------------------------------------
bool OscDssiIF::oscInitGui()
{
  if(!_oscSynthIF)
    return false;
    
  return OscIF::oscInitGui(QT_TRANSLATE_NOOP("@default", "dssi_synth"), _oscSynthIF->dssiSynth()->baseName(), 
                           _oscSynthIF->dssiSynth()->name(), _oscSynthIF->dssiSynthI()->name(), 
                           //_oscSynthIF->dssiSynth()->filePath(), _oscSynthIF->dssiSynth()->path());
                           _oscSynthIF->dssiSynth()->fileName(), _oscSynthIF->dssi_ui_filename());    // p4.0.19
}
      
#endif   // DSSI_SUPPORT
      

//---------------------------------------------------------
//   OscEffectIF::
//   oscSetPluginI
//---------------------------------------------------------

void OscEffectIF::oscSetPluginI(PluginI* s)
{ 
  _oscPluginI = s; 
  //if(_oscControlFifos)
  //  delete[] _oscControlFifos;
  //_oscControlFifos = 0;
    
  //if(_oscPluginI && _oscPluginI->plugin())
  //{
  //  unsigned long ports = _oscPluginI->plugin()->controlInPorts();
  //  _oscControlFifos = new OscControlFifo[ports];  
  //}  
}

//---------------------------------------------------------
//   oscUpdate
//---------------------------------------------------------

int OscEffectIF::oscUpdate(lo_arg** argv)
{
  // Make sure to call base method.
  OscIF::oscUpdate(argv);
  
  // Send project directory. No, done in PluginI.
  //lo_send(_uiOscTarget, _uiOscConfigurePath, "ss",
  //   DSSI_PROJECT_DIRECTORY_KEY, museProject.toLatin1().constData());  // song->projectPath()
  
  if(_oscPluginI)
    _oscPluginI->oscUpdate();
  
  return 0;
}

//---------------------------------------------------------
//   oscConfigure
//---------------------------------------------------------

int OscEffectIF::oscConfigure(lo_arg** argv)
{
  //OscIF::oscConfigure(argv);
  
  if(_oscPluginI)
    _oscPluginI->oscConfigure((const char*)&argv[0]->s, (const char*)&argv[1]->s);
  
  return 0;
}

//---------------------------------------------------------
//   oscControl
//---------------------------------------------------------

int OscEffectIF::oscControl(lo_arg** argv)
{
  //OscIF::oscControl(argv);
  
  int port = argv[0]->i;
  if(port < 0)
    return 0;
    
  if(_oscPluginI)
    _oscPluginI->oscControl(argv[0]->i, argv[1]->f);
  
  return 0;
}

//---------------------------------------------------------
//   oscInitGui
//---------------------------------------------------------
bool OscEffectIF::oscInitGui()
{
  if(!_oscPluginI)
    return false;
    
  return OscIF::oscInitGui(QT_TRANSLATE_NOOP("@default", "ladspa_efx"), _oscPluginI->plugin()->lib(false), 
                           _oscPluginI->plugin()->label(), _oscPluginI->label(), 
                           //_oscPluginI->plugin()->filePath(), _oscPluginI->plugin()->dirPath(false));
                           _oscPluginI->plugin()->fileName(), _oscPluginI->dssi_ui_filename());    // p4.0.19
}
      

#else //OSC_SUPPORT
void initOSC() {}
void exitOSC() {}

#endif
