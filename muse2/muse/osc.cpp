//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id: osc.cpp,v 1.0.0.0 2010/04/22 03:39:58 terminator356 Exp $
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

#ifdef OSC_SUPPORT

// Turn on debugging messages
//#define OSC_DEBUG 

// Whether to use a QProcess or fork + execlp to start the gui. (Note fork + execlp give problems - zombies when synth window closed.)
#define _USE_QPROCESS_FOR_GUI_ 1

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include <QFileInfo>
#include <QString>
#include <QStringList>

#ifdef _USE_QPROCESS_FOR_GUI_
  #include <QProcess>
#else
  #include <unistd.h> 
  #include <signal.h>
#endif

#include <lo/lo.h>

#ifdef DSSI_SUPPORT
#include "dssihost.h"
#endif

#include "stringparam.h"
#include "plugin.h"
#include "track.h"
#include "song.h"
#include "synth.h"
#include "app.h"
#include "globals.h"
#include "globaldefs.h"

#endif   // OSC_SUPPORT

namespace MusECore {

#ifdef OSC_SUPPORT
  
static lo_server_thread serverThread = 0;
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

  TrackList* tl = MusEGlobal::song->tracks();
  

  #ifdef OSC_DEBUG 
  if(isSynth)
    fprintf(stderr, "oscMessageHandler: got message for dssi synth...\n");
  else  
    fprintf(stderr, "oscMessageHandler: got message for ladspa effect...\n");
  #endif
    
  
  #ifdef DSSI_SUPPORT
  if(isSynth)
  {
    // Message is meant for a dssi synth. Check dssi synth instances...
    SynthIList* sl = MusEGlobal::song->syntis();
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
      
      // TODO: Fix this dynamic cast - it may be a slowdown.
      DssiSynthIF* instance = dynamic_cast<DssiSynthIF*>(synti->sif());
      if(!instance)
        break;
        
      p = sub + ba.length();
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
        
        QByteArray ba = instance->label().toLatin1();
        const char* sub = strstr(p, ba.constData());
        if(sub == NULL) 
          continue;
          
        Plugin* plugin = instance->plugin();
        if(!plugin)
          break;
        
        p = sub + ba.length();
        if (*p != '/' || *(p + 1) == 0)
        {
          fprintf(stderr, "oscMessageHandler: error: effect: end of path or no /\n");
          return oscDebugHandler(path, types, argv, argc, data, user_data);
        }
              
        ++p;
  
        #ifdef OSC_DEBUG 
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
  
  url = lo_server_thread_get_url(serverThread);
  if(!url)
  {
    lo_server_thread_free(serverThread);
    printf("initOSC() Failed to get OSC server thread url !\n");
    return;
  }
  
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
        
//---------------------------------------------------------
//   OscIF
//   Open Sound Control Interface
//---------------------------------------------------------

OscIF::OscIF()
{
  _uiOscTarget = 0;
  _uiOscSampleRatePath = 0;
  _uiOscShowPath = 0;
  _uiOscControlPath = 0;
  _uiOscConfigurePath = 0;
  _uiOscProgramPath = 0;
  _uiOscPath = 0;
#ifdef _USE_QPROCESS_FOR_GUI_
  _oscGuiQProc = 0;
#else  
  _guiPid = -1;
#endif
  _oscGuiVisible = false;
  
  old_prog=old_bank=0xDEADBEEF;
  old_control=NULL;
  control_port_mapper=NULL;
  maxDssiPort=0;
}

OscIF::~OscIF()
{
#ifdef _USE_QPROCESS_FOR_GUI_
  if(_oscGuiQProc)
  {
    if(_oscGuiQProc->state())
    {
      #ifdef OSC_DEBUG 
      printf("OscIF::~OscIF terminating _oscGuiQProc\n");
      #endif
      
      _oscGuiQProc->terminate();
      _oscGuiQProc->waitForFinished(3000);
    }  
    delete _oscGuiQProc;
  }
  
#else  // NOT  _USE_QPROCESS_FOR_GUI_

  if (_guiPid != -1)
  {  
    if(kill(_guiPid, SIGHUP) != -1)
      _guiPid = -1;
  }      
#endif  // _USE_QPROCESS_FOR_GUI_

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
    
  if (old_control)
    delete [] old_control;
}

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
      printf(" museProject:%s\n", MusEGlobal::museProject.toLatin1().constData());
      #endif
      
      // Send sample rate.
      lo_send(_uiOscTarget, _uiOscSampleRatePath, "i", MusEGlobal::sampleRate);
      
      // DELETETHIS 46
      // Send project directory.
      //lo_send(_uiOscTarget, _uiOscConfigurePath, "ss",
      //   DSSI_PROJECT_DIRECTORY_KEY, museProject.toLatin1().constData());  // MusEGlobal::song->projectPath()
      
      // Done in sub-classes.
      /*
      #ifdef DSSI_SUPPORT
      //lo_send(_uiOscTarget, _uiOscConfigurePath, "ss",
         //DSSI_PROJECT_DIRECTORY_KEY, MusEGlobal::song->projectPath().toAscii().data());
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
     // DELETETHIS 22  
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
      
// DELETETHIS 52
// Just an attempt to really kill the process, an attempt to fix gui not re-showing after closing. Doesn't help.
/*
#ifdef _USE_QPROCESS_FOR_GUI_
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
          _oscGuiQProc->waitForFinished(3000);
        }  
        //delete _oscGuiQProc;
        //_oscGuiQProc = 0;
      }
      
     
#else  // NOT  _USE_QPROCESS_FOR_GUI_        

      if(_guiPid != -1)
      {  
        #ifdef OSC_DEBUG 
        printf("OscIF::oscExiting hanging up _guiPid:%d\n", _guiPid);
        #endif
        //if(kill(_guiPid, SIGHUP) != -1)
        //if(kill(_guiPid, SIGTERM) != -1)
        if(kill(_guiPid, SIGKILL) != -1)
        {  
          #ifdef OSC_DEBUG 
          printf(" hang up sent\n");
          #endif
          _guiPid = -1;
        }  
      }  
      
#endif // _USE_QPROCESS_FOR_GUI_
*/
      
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
        
      // DELETETHIS 20
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
  
// DELETETHIS 37
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

void OscIF::oscSendProgram(unsigned long prog, unsigned long bank, bool force)
{
  if(_uiOscTarget && _uiOscProgramPath &&
     (bank!=old_bank || prog!=old_prog || force) )
  {
    lo_send(_uiOscTarget, _uiOscProgramPath, "ii", bank, prog);
    old_bank=bank;
    old_prog=prog;
  }
}

//---------------------------------------------------------
//   oscSendControl
//---------------------------------------------------------

void OscIF::oscSendControl(unsigned long dssiPort, float v, bool force)
{
  if(_uiOscTarget && _uiOscControlPath &&
     ((dssiPort<maxDssiPort && old_control[control_port_mapper->at(dssiPort)]!=v) || force) )
  {
    lo_send(_uiOscTarget, _uiOscControlPath, "if", dssiPort, v);
    old_control[control_port_mapper->at(dssiPort)]=v;
  }
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

bool OscIF::oscInitGui(const QString& typ, const QString& baseName, const QString& name, 
                       const QString& label, const QString& filePath, const QString& guiPath,
                       const std::vector<unsigned long>* control_port_mapper_)
{
      if (old_control==NULL)
      {
        control_port_mapper=control_port_mapper_;
        
        unsigned long nDssiPorts=0;
        for (unsigned i=0;i<control_port_mapper->size();i++)
          if (control_port_mapper->at(i)!=(unsigned long)-1 && control_port_mapper->at(i)+1 > nDssiPorts)
            nDssiPorts=control_port_mapper->at(i)+1;
        
        old_control=new float[nDssiPorts];
        for (unsigned long i=0;i<nDssiPorts;i++) // init them all with "not a number"
          old_control[i]=NAN;

        maxDssiPort=nDssiPorts;
      }
      else
      {
        control_port_mapper=control_port_mapper_;
        
        unsigned long nDssiPorts=0;
        for (unsigned i=0;i<control_port_mapper->size();i++)
          if (control_port_mapper->at(i)!=(unsigned long)-1 && control_port_mapper->at(i)+1 > nDssiPorts)
            nDssiPorts=control_port_mapper->at(i)+1;
        
        if (maxDssiPort!=nDssiPorts)
        {
          // this should never happen, right?
          printf("STRANGE: nDssiPorts has changed (old=%lu, now=%lu)!\n", maxDssiPort, nDssiPorts);
          delete [] old_control;
          old_control=new float[nDssiPorts];
          for (unsigned long i=0;i<nDssiPorts;i++) // init them all with "not a number"
            old_control[i]=NAN;
          maxDssiPort=nDssiPorts;
        }
      }
      
      // Are we already running? We don't want to allow another process do we...
#ifdef _USE_QPROCESS_FOR_GUI_
      if((_oscGuiQProc != 0) && (_oscGuiQProc->state()))
        return true;
#else
      if(_guiPid != -1)
        return true;
#endif
        
      #ifdef OSC_DEBUG 
      fprintf(stderr, "OscIF::oscInitGui\n");
      #endif

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
            
      QString oscUrl;
      oscUrl = QString("%1%2/%3/%4").arg(QString( url)).arg(typ).arg(baseName).arg(label);
      
                        
#ifdef _USE_QPROCESS_FOR_GUI_
      
      // fork + execlp cause the process to remain (zombie) after closing gui, requiring manual kill.
      // Using QProcess works OK. 
      // No QProcess created yet? Do it now. Only once per SynthIF instance. Exists until parent destroyed.
      if(_oscGuiQProc == 0)
        _oscGuiQProc = new QProcess();                        
      
      QString program(guiPath);
      QStringList arguments;
      arguments << oscUrl
                << filePath
                << name
                << (titlePrefix() + label);

      #ifdef OSC_DEBUG 
      fprintf(stderr, "OscIF::oscInitGui starting QProcess\n");
      #endif
      _oscGuiQProc->start(program, arguments);
        
      if(_oscGuiQProc->state())
      {
        #ifdef OSC_DEBUG 
        fprintf(stderr, "OscIF::oscInitGui started QProcess\n");
        #endif
      }
      else
      {
        fprintf(stderr, "exec %s %s %s %s failed: %s\n",
                guiPath.toLatin1().constData(),
                oscUrl.toLatin1().constData(),
                filePath.toLatin1().constData(),
                name.toLatin1().constData(),
                strerror(errno));
      }
      
      #ifdef OSC_DEBUG 
      fprintf(stderr, "OscIF::oscInitGui after QProcess\n");
      #endif
      
#else  // NOT  _USE_QPROCESS_FOR_GUI_
                                
      #ifdef OSC_DEBUG 
      fprintf(stderr, "forking...\n");
      #endif

      QString guiName = QFileInfo(guiPath).fileName();
      // Note: fork + execlp cause the process to remain (zombie) after closing gui, requiring manual kill. Use QProcess instead.
      if((_guiPid = fork()) == 0)  
      {
         execlp(
                 guiPath.toLatin1().constData(),
                 guiName.toLatin1().constData(),
                 oscUrl.toLatin1().constData(),
                 filePath.toLatin1().constData(),
                 name.toLatin1().constData(),
                 //"channel 1", (void*)0);
                 label.toLatin1().constData(), (void*)0);

        // Should not return after execlp. If so it's an error.
        fprintf(stderr, "exec %s %s %s %s %s failed: %s\n",
                guiPath.toLatin1().constData(),
                guiName.toLatin1().constData(),
                oscUrl.toLatin1().constData(),
                filePath.toLatin1().constData(),
                name.toLatin1().constData(),
                strerror(errno));
        //exit(1);
      }
      
#endif   // _USE_QPROCESS_FOR_GUI_
      
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
      
#ifdef _USE_QPROCESS_FOR_GUI_
      if((_oscGuiQProc == 0) || (!_oscGuiQProc->state()))
#else        
      if(_guiPid == -1)
#endif        
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
      
      for (int i = 0; i < 20; ++i) {
            if (_uiOscPath)
                  break;
            sleep(1);
            }
      if (_uiOscPath == 0) {
            printf("OscIF::oscShowGui(): no _uiOscPath. Error: Timeout - synth gui did not start within 20 seconds.\n");
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

void OscDssiIF::oscSetSynthIF(DssiSynthIF* s)
{ 
  _oscSynthIF = s;
}

//---------------------------------------------------------
//   oscUpdate
//---------------------------------------------------------

int OscDssiIF::oscUpdate(lo_arg **argv)
{
      // Make sure to call base method.
      OscIF::oscUpdate(argv);
      
      // Send sample rate. No, done in base class. DELETETHIS 7
      //lo_send(_uiOscTarget, _uiOscSampleRatePath, "i", sampleRate);
      
      // Send project directory. No, done in DssiSynthIF.
      //lo_send(_uiOscTarget, _uiOscConfigurePath, "ss",
      //   DSSI_PROJECT_DIRECTORY_KEY, museProject.toLatin1().constData());  // MusEGlobal::song->projectPath()
      
      if(_oscSynthIF)
        _oscSynthIF->oscUpdate();
// DELETETHIS 23
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
  if(_oscSynthIF)
    _oscSynthIF->oscConfigure((const char*)&argv[0]->s, (const char*)&argv[1]->s);
  return 0;
}

//---------------------------------------------------------
//   oscMidi
//---------------------------------------------------------

int OscDssiIF::oscMidi(lo_arg** argv)
{
  if(_oscSynthIF)
    _oscSynthIF->oscMidi(argv[0]->m[1], argv[0]->m[2], argv[0]->m[3]);
  
  return 0;
}

//---------------------------------------------------------
//   oscProgram
//---------------------------------------------------------

int OscDssiIF::oscProgram(lo_arg** argv)
{
  if(_oscSynthIF)
  {
    _oscSynthIF->oscProgram(argv[1]->i, argv[0]->i);
    old_prog=argv[1]->i;
    old_bank=argv[0]->i;
  }
  
  return 0;
}

//---------------------------------------------------------
//   oscControl
//---------------------------------------------------------

int OscDssiIF::oscControl(lo_arg** argv)
{
  int port = argv[0]->i;
  if(port < 0)
    return 0;
    
  if(_oscSynthIF)
  {
    _oscSynthIF->oscControl(argv[0]->i, argv[1]->f);
    if (port<(int)maxDssiPort)
      old_control[control_port_mapper->at(port)]=argv[1]->f;
  }
  
  return 0;
}

//---------------------------------------------------------
//   oscInitGui
//---------------------------------------------------------
bool OscDssiIF::oscInitGui()
{
  if(!_oscSynthIF)
    return false;
  
  return OscIF::oscInitGui("dssi_synth", _oscSynthIF->dssiSynth()->baseName(), 
                           _oscSynthIF->dssiSynth()->name(), _oscSynthIF->dssiSynthI()->name(), 
                           _oscSynthIF->dssiSynth()->fileName(), _oscSynthIF->dssi_ui_filename(),
                           _oscSynthIF->dssiSynth()->getRpIdx());
}

QString OscDssiIF::titlePrefix() const 
{ 
  return _oscSynthIF ? _oscSynthIF->titlePrefix() : QString(); 
}
      
#endif   // DSSI_SUPPORT
      
//---------------------------------------------------------
//   OscEffectIF::
//   oscSetPluginI
//---------------------------------------------------------

void OscEffectIF::oscSetPluginI(PluginI* s)
{ 
  _oscPluginI = s; 
}

//---------------------------------------------------------
//   oscUpdate
//---------------------------------------------------------

int OscEffectIF::oscUpdate(lo_arg** argv)
{
  // Make sure to call base method.
  OscIF::oscUpdate(argv);
  
  if(_oscPluginI)
    _oscPluginI->oscUpdate();
  
  return 0;
}

//---------------------------------------------------------
//   oscConfigure
//---------------------------------------------------------

int OscEffectIF::oscConfigure(lo_arg** argv)
{
  if(_oscPluginI)
    _oscPluginI->oscConfigure((const char*)&argv[0]->s, (const char*)&argv[1]->s);
  
  return 0;
}

//---------------------------------------------------------
//   oscControl
//---------------------------------------------------------

int OscEffectIF::oscControl(lo_arg** argv)
{
  int port = argv[0]->i;
  if(port < 0)
    return 0;
    
  if(_oscPluginI)
  {
    _oscPluginI->oscControl(argv[0]->i, argv[1]->f);
    if (port<(int)maxDssiPort)
      old_control[control_port_mapper->at(port)]=argv[1]->f;
  }
  
  return 0;
}

//---------------------------------------------------------
//   oscInitGui
//---------------------------------------------------------
bool OscEffectIF::oscInitGui()
{
  if(!_oscPluginI)
    return false;
    
  return OscIF::oscInitGui("ladspa_efx", _oscPluginI->plugin()->lib(false), 
                           _oscPluginI->plugin()->label(), _oscPluginI->label(), 
                           _oscPluginI->plugin()->fileName(), _oscPluginI->dssi_ui_filename(),
                           _oscPluginI->plugin()->getRpIdx());  
}
      
QString OscEffectIF::titlePrefix() const 
{ 
  return _oscPluginI ? _oscPluginI->titlePrefix() : QString(); 
}


#else //OSC_SUPPORT
void initOSC() {}
void exitOSC() {}
#endif

} // namespace MusECore
