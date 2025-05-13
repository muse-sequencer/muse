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
#ifndef _USE_QPROCESS_FOR_GUI_
#include <errno.h>
#endif
#include "muse_math.h"

//#include <QFileInfo>
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
//#include "app.h"
#include "globals.h"
//#include "globaldefs.h"

#endif   // OSC_SUPPORT

namespace MusECore {

#ifdef OSC_SUPPORT
  
static lo_server_thread serverThread = 0;
static char* url = 0;
static bool oscServerRunning = false;

//----------------------------------------------------
// From official OSC specs:
// Printable ASCII characters not allowed in names of OSC Methods or OSC Containers:
// character 	name 	ASCII code (decimal)
// ’ ’ 	space
// # 	number sign 	35
// * 	asterisk 	42
// , 	comma 	44
// / 	forward slash 	47
// ? 	question mark 	63
// [ 	open bracket 	91
// ] 	close bracket 	93
// { 	open curly brace 	123
// } 	close curly brace 	125
//----------------------------------------------------
// Some string helpers. Currently not used.
// But hey, if text is needed in the url, they'll help.
//----------------------------------------------------
#if 0
static void stringToOscUrl(QString &s)
{
  // We also replace % since we use it.
  s.replace('%', "%37");
  s.replace(' ', "%20");
  s.replace('#', "%35");
  s.replace('*', "%42");
  s.replace(',', "%44");
  s.replace('/', "%47");
  s.replace('?', "%63");
  s.replace('[', "%91");
  s.replace(']', "%93");
  s.replace('{', "%123");
  s.replace('}', "%125");
}

static void oscUrlToString(QString &s)
{
  s.replace("%20", " ");
  s.replace("%35", "#");
  s.replace("%42", "*");
  s.replace("%44", ",");
  s.replace("%47", "/");
  s.replace("%63", "?");
  s.replace("%91", "[");
  s.replace("%93", "]");
  s.replace("%123", "{");
  s.replace("%125", "}");
  // We also restore % since we use it.
  s.replace("%37", "%");
}
#endif

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
      fprintf(stderr, "MusE: got unhandled OSC message:\n   path: <%s>\n", path);
      for (int i = 0; i < argc; i++) {
            fprintf(stderr, "   arg %d '%c' ", i, types[i]);
            lo_arg_pp(lo_type(types[i]), argv[i]);
            fprintf(stderr, "\n");
            }
      return 1;
      }

//---------------------------------------------------------
//   oscMessageHandler
//---------------------------------------------------------

int oscMessageHandler(const char* path, const char* types, lo_arg** argv,
   int argc, lo_message data, void* user_data)
{
  const QString sp(path);
  const QString stypes(types);

  // NOTE: Tried this, always returns 0 sec and 1 fractional. Shame, looks like timestamps are not used.
  //lo_timetag lo_tt = lo_message_get_timestamp(data);

  #ifdef OSC_DEBUG
  if(argc)
  {
      fprintf(stderr, "oscMessageHandler: path:%s argc:%d\n", path, argc);
      for(int i = 0; i < argc; ++i)
      {
        fprintf(stderr, " ");
        lo_arg_pp((lo_type)types[i], argv[i]);
      }
      fprintf(stderr, "\n");
  }
  else
  {
      fprintf(stderr, "%s\n", path);
      fprintf(stderr, "oscMessageHandler: no args, path:%s\n", path);
  }
  #endif

  const QStringList sl = sp.split('/');

  #if defined(DSSI_SUPPORT) || defined(OSC_DEBUG)
  bool isSynth = false;
  #endif

  #ifdef DSSI_SUPPORT
  if(sl.at(1) == "dssi_synth")
    isSynth = true;
  else
  #endif
  if(sl.at(1) == "dssi_efx")
  {
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

  bool ok;

  const int urltrackno = sl.at(2).toInt(&ok);
  if(!ok)
  {
    fprintf(stderr, "oscMessageHandler: error: Invalid track number text\n");
    return oscDebugHandler(path, types, argv, argc, data, user_data);
  }

  if(urltrackno >= (int)tl->size())
  {
    fprintf(stderr, "oscMessageHandler: error: Track number out of range\n");
    return oscDebugHandler(path, types, argv, argc, data, user_data);
  }

  Track *track = tl->at(urltrackno);

  if(track->isMidiTrack())
  {
    fprintf(stderr, "oscMessageHandler: error: Track is a midi track\n");
    return oscDebugHandler(path, types, argv, argc, data, user_data);
  }

  AudioTrack *atrack = static_cast<AudioTrack*>(track);

  #ifdef DSSI_SUPPORT
  if(isSynth)
  {
    if(!atrack->isSynthTrack())
    {
      fprintf(stderr, "oscMessageHandler: error: synth: Track is not a synth track\n");
      return oscDebugHandler(path, types, argv, argc, data, user_data);
    }

    SynthI* synti = static_cast<SynthI*>(atrack);

    if(!synti->sif() || !synti->synth() ||
       (synti->synth()->pluginType() != MusEPlugin::PluginTypeDSSI &&
        synti->synth()->pluginType() != MusEPlugin::PluginTypeDSSIVST))
    {
      fprintf(stderr, "oscMessageHandler: error: synth: No sif or no synth or synth is not DSSI\n");
      return oscDebugHandler(path, types, argv, argc, data, user_data);
    }

    DssiSynthIF* instance = static_cast<DssiSynthIF*>(synti->sif());

    const QString &urlcommand = sl.at(3);
    // Shouldn't be required.
    //oscUrlToString(urlcommand);

    #ifdef OSC_DEBUG
    fprintf(stderr, "oscMessageHandler: synth track:%s method:%s\n",
            synti->name().toLocal8Bit().constData(), urlcommand.toLocal8Bit().constData());
    #endif

    OscIF& oscif = instance->oscIF();

    if (urlcommand == "configure" && argc == 2 && stypes == "ss")
          return oscif.oscConfigure(argv);
    else if (urlcommand == "control" && argc == 2 && stypes == "if")
          return oscif.oscControl(argv);
    else if (urlcommand == "midi" && argc == 1 && stypes == "m")
          return oscif.oscMidi(argv);
    else if (urlcommand == "program" && argc == 2 && stypes == "ii")
          return oscif.oscProgram(argv);
    else if (urlcommand == "update" && argc == 1 && stypes == "s")
          return oscif.oscUpdate(argv);
    else if (urlcommand == "exiting" && argc == 0)
          return oscif.oscExiting(argv);
    fprintf(stderr, "oscMessageHandler: synth: unknown command\n");
    return oscDebugHandler(path, types, argv, argc, data, user_data);
  }
  else
  #endif //DSSI_SUPPORT
  {
    const int urlrackpos = sl.at(3).toInt(&ok);
    if(!ok)
    {
      fprintf(stderr, "oscMessageHandler: error: effect: Invalid rack position number text\n");
      return oscDebugHandler(path, types, argv, argc, data, user_data);
    }

    if(urlrackpos >= PipelineDepth)
    {
      fprintf(stderr, "oscMessageHandler: error: effect: Rack position number out of range\n");
      return oscDebugHandler(path, types, argv, argc, data, user_data);
    }

    const Pipeline* efxPipe = atrack->efxPipe();
    if(!efxPipe)
    {
      fprintf(stderr, "oscMessageHandler: error: effect: Track has no effect rack\n");
      return oscDebugHandler(path, types, argv, argc, data, user_data);
    }

    PluginI* instance = efxPipe->at(urlrackpos);
    if(!instance)
    {
      fprintf(stderr, "oscMessageHandler: error: effect: No plugin at given rack position\n");
      return oscDebugHandler(path, types, argv, argc, data, user_data);
    }

    if(!instance->plugin())
    {
      fprintf(stderr, "oscMessageHandler: error: effect: Plugin instance has no plugin\n");
      return oscDebugHandler(path, types, argv, argc, data, user_data);
    }

    const QString &urlcommand = sl.at(4);
    // Shouldn't be required.
    //oscUrlToString(urlcommand);

    #ifdef OSC_DEBUG
    fprintf(stderr, "oscMessageHandler: effect:%s method:%s\n",
            instance->name().toLocal8Bit().constData(), urlcommand.toLocal8Bit().constData());
    #endif

    OscIF& oscif = instance->oscIF();

    if (urlcommand == "configure" && argc == 2 && stypes == "ss")
          return oscif.oscConfigure(argv);
    else if (urlcommand == "control" && argc == 2 && stypes == "if")
          return oscif.oscControl(argv);
    else if (urlcommand == "midi" && argc == 1 && stypes == "m")
          return oscif.oscMidi(argv);
    else if (urlcommand == "program" && argc == 2 && stypes == "ii")
          return oscif.oscProgram(argv);
    else if (urlcommand == "update" && argc == 1 && stypes == "s")
          return oscif.oscUpdate(argv);
    else if (urlcommand == "exiting" && argc == 0)
          return oscif.oscExiting(argv);
    fprintf(stderr, "oscMessageHandler: error: effect: unknown command\n");
    return oscDebugHandler(path, types, argv, argc, data, user_data);
  }
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
      fprintf(stderr, "initOSC() Failed to create OSC server!\n");
      return;
    }
  }  
  
  url = lo_server_thread_get_url(serverThread);
  if(!url)
  {
    lo_server_thread_free(serverThread);
    fprintf(stderr, "initOSC() Failed to get OSC server thread url !\n");
    return;
  }
  
  lo_method meth = 0;
  meth = lo_server_thread_add_method(serverThread, 0, 0, oscMessageHandler, 0);
  if(!meth)
  {
    fprintf(stderr, "initOSC() Failed to add oscMessageHandler method to OSC server!\n");
    // Does not return a value.
    lo_server_thread_free(serverThread);
    serverThread = 0;
    free(url);
    url = 0;
    return;
  }
  
  #ifdef OSC_DEBUG 
  fprintf(stderr, "initOSC() url:%s\n", url);
  #endif
  
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
    serverThread = 0;
  }  
  if(url)
  {
    free(url);
    url = 0;
  }
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
  _uiOscTarget = nullptr;
  _uiOscSampleRatePath = nullptr;
  _uiOscShowPath = nullptr;
  _uiOscHidePath = nullptr;
  _uiOscQuitPath = nullptr;
  _uiOscControlPath = nullptr;
  _uiOscConfigurePath = nullptr;
  _uiOscProgramPath = nullptr;
  _uiOscPath = nullptr;
#ifdef _USE_QPROCESS_FOR_GUI_
  _oscGuiQProc = nullptr;
#else  
  _guiPid = -1;
#endif
  _oscGuiVisible = false;
  
  old_prog=old_bank=0xDEADBEEF;
  old_control=nullptr;
  control_port_mapper=nullptr;
  maxDssiPort=0;
}

OscIF::~OscIF()
{
#ifdef _USE_QPROCESS_FOR_GUI_
  if(_oscGuiQProc)
  {
    if(_oscGuiQProc->state() != QProcess::NotRunning)
    {
      #ifdef OSC_DEBUG 
      fprintf(stderr, "OscIF::~OscIF terminating _oscGuiQProc\n");
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
  if(_uiOscHidePath)
    free(_uiOscHidePath);
  if(_uiOscQuitPath)
    free(_uiOscQuitPath);
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

bool OscIF::isRunning() const
{
  return _oscGuiQProc && _oscGuiQProc->state() == QProcess::Running;
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
      _uiOscShowPath = (char *)malloc(pl + 6);
      sprintf(_uiOscShowPath, "%s/show", _uiOscPath);

      if (_uiOscHidePath)
            free(_uiOscHidePath);
      _uiOscHidePath = (char *)malloc(pl + 6);
      sprintf(_uiOscHidePath, "%s/hide", _uiOscPath);

      if (_uiOscQuitPath)
            free(_uiOscQuitPath);
      _uiOscQuitPath = (char *)malloc(pl + 6);
      sprintf(_uiOscQuitPath, "%s/quit", _uiOscPath);

      /* At this point a more substantial host might also call
      * configure() on the UI to set any state that it had remembered
      * for the plugin instance.  But we don't remember state for
      * plugin instances (see our own configure() implementation in
      * osc_configure_handler), and so we have nothing to send except
      * the optional project directory.
      */

      #ifdef OSC_DEBUG 
      fprintf(stderr, "OscIF::oscUpdate: _uiOscTarget:%p\n", _uiOscTarget);
      if(url)
        fprintf(stderr, " server url:%s\n", url);
      else  
        fprintf(stderr, " no server url!\n");
      fprintf(stderr, " update path:%s\n", purl);
      fprintf(stderr, " _uiOscPath:%s\n", _uiOscPath);
      fprintf(stderr, " _uiOscSampleRatePath:%s\n", _uiOscSampleRatePath);
      fprintf(stderr, " _uiOscConfigurePath:%s\n", _uiOscConfigurePath);
      fprintf(stderr, " _uiOscProgramPath:%s\n", _uiOscProgramPath);
      fprintf(stderr, " _uiOscControlPath:%s\n",_uiOscControlPath);
      fprintf(stderr, " _uiOscShowPath:%s\n", _uiOscShowPath);
      fprintf(stderr, " _uiOscHidePath:%s\n", _uiOscHidePath);
      fprintf(stderr, " _uiOscQuitPath:%s\n", _uiOscQuitPath);
      fprintf(stderr, " museProject:%s\n", MusEGlobal::museProject.toLocal8Bit().constData());
      #endif
      
      // Send sample rate.
      lo_send(_uiOscTarget, _uiOscSampleRatePath, "i", MusEGlobal::sampleRate);
      
      // DELETETHIS 46
      // Send project directory.
      //lo_send(_uiOscTarget, _uiOscConfigurePath, "ss",
      //   DSSI_PROJECT_DIRECTORY_KEY, museProject.toUtf8().constData());  // MusEGlobal::song->projectPath()
      
      // Done in sub-classes.
      /*
      #ifdef DSSI_SUPPORT
      //lo_send(_uiOscTarget, _uiOscConfigurePath, "ss",
         //DSSI_PROJECT_DIRECTORY_KEY, MusEGlobal::song->projectPath().toUtf8().data());
      lo_send(_uiOscTarget, _uiOscConfigurePath, "ss",
         DSSI_PROJECT_DIRECTORY_KEY, museProject.toUtf8().constData());
      
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

int OscIF::oscProgram(lo_arg**)   { return 0; }
int OscIF::oscControl(lo_arg**)   { return 0; }

//---------------------------------------------------------
//   oscExiting
//---------------------------------------------------------

int OscIF::oscExiting(lo_arg**)
{
  // WARNING: According to the DSSI document RFC.txt:
  //
  // <base path>/exiting
  //  Notifies the host that the UI is in the process of exiting, for
  //   example if the user closed the GUI window using the window manager.
  //  The UI should not send this if exiting in response to a quit
  //   message (see below).  No arguments.  (required method)
  //
  // <base path>/quit
  //  Exit the UI.  The UI should not send any more communication to the
  //   host about this plugin after receiving a quit message.  It may save
  //   any of its own state before exiting, but it should not retain state
  //   that may be necessary for the host to restore the plugin instance
  //   correctly.  (required method)
  //
  // But... some plugins were actually observed calling 'exiting' in response to quit.
  // So be careful...

  oscCleanupGui();

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

bool OscIF::oscInitGui(const QString& typ, /*QString baseName,*/ QString pluginLabel,
                       int trackno, const QString& filePath, const QString& guiPath,
                       const std::vector<unsigned long>* control_port_mapper_, int rackpos)
{
      if (old_control==nullptr)
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
          fprintf(stderr, "STRANGE: nDssiPorts has changed (old=%lu, now=%lu)!\n", maxDssiPort, nDssiPorts);
          delete [] old_control;
          old_control=new float[nDssiPorts];
          for (unsigned long i=0;i<nDssiPorts;i++) // init them all with "not a number"
            old_control[i]=NAN;
          maxDssiPort=nDssiPorts;
        }
      }

      // Are we already running? We don't want to allow another process do we...
#ifdef _USE_QPROCESS_FOR_GUI_
      if((_oscGuiQProc != 0) && (_oscGuiQProc->state() != QProcess::NotRunning))
        return false;
#else
      if(_guiPid != -1)
        return false;
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

      // WARNING: It seems spaces are NOT allowed in the URL.
      //          This was verified by putting spaces in a track name before opening the UI.
      // From official OSC specs:
      // Printable ASCII characters not allowed in names of OSC Methods or OSC Containers:
      // character 	name 	ASCII code (decimal)
      // ’ ’ 	space
      // # 	number sign 	35
      // * 	asterisk 	42
      // , 	comma 	44
      // / 	forward slash 	47
      // ? 	question mark 	63
      // [ 	open bracket 	91
      // ] 	close bracket 	93
      // { 	open curly brace 	123
      // } 	close curly brace 	125

      QString oscUrl;
      if(rackpos != -1)
        oscUrl = QString("%1%2/%3/%4").arg(QString( url)).arg(typ).arg(trackno).arg(rackpos);
      else
        oscUrl = QString("%1%2/%3").arg(QString( url)).arg(typ).arg(trackno);


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
                << pluginLabel
                << displayName();

      #ifdef OSC_DEBUG
      fprintf(stderr, "OscIF::oscInitGui starting QProcess\n");
      #endif

      _oscGuiQProc->start(program, arguments);

      if(_oscGuiQProc->waitForStarted(10000)) // 10 secs.
      {
        #ifdef OSC_DEBUG
        fprintf(stderr, "OscIF::oscInitGui started QProcess\n");
        fprintf(stderr, "guiPath:%s oscUrl:%s filePath:%s pluginLabel:%s\n",
                guiPath.toLocal8Bit().constData(),
                oscUrl.toLocal8Bit().constData(),
                filePath.toLocal8Bit().constData(),
                pluginLabel.toLocal8Bit().constData());
        #endif
      }
      else
      {
        fprintf(stderr, "exec %s %s %s %s failed: %s\n",
                guiPath.toLocal8Bit().constData(),
                oscUrl.toLocal8Bit().constData(),
                filePath.toLocal8Bit().constData(),
                pluginLabel.toLocal8Bit().constData(),
                _oscGuiQProc->errorString().toLocal8Bit().constData());
        return false;
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
                 guiPath.toLocal8Bit().constData(),
                 guiName.toLocal8Bit().constData(),
                 oscUrl.toLocal8Bit().constData(),
                 filePath.toLocal8Bit().constData(),
                 pluginLabel.toLocal8Bit().constData(),
                 //"channel 1", (void*)0);
                 name.toLocal8Bit().constData(), (void*)0);

        // Should not return after execlp. If so it's an error.
        fprintf(stderr, "exec %s %s %s %s %s failed: %s\n",
                guiPath.toLocal8Bit().constData(),
                guiName.toLocal8Bit().constData(),
                oscUrl.toLocal8Bit().constData(),
                filePath.toLocal8Bit().constData(),
                pluginLabel.toLocal8Bit().constData(),
                strerror(errno));
        //exit(1);
        return false;
      }

#endif   // _USE_QPROCESS_FOR_GUI_

  return true;
}

void OscIF::oscCleanupGui()
{
  // The gui is gone now, right?
  _oscGuiVisible = false;

  if(_uiOscTarget)
    lo_address_free(_uiOscTarget);
  _uiOscTarget = nullptr;
  if(_uiOscSampleRatePath)
    free(_uiOscSampleRatePath);
  _uiOscSampleRatePath = nullptr;
  if(_uiOscShowPath)
    free(_uiOscShowPath);
  _uiOscShowPath = nullptr;
  if(_uiOscHidePath)
    free(_uiOscHidePath);
  _uiOscHidePath = nullptr;
  if(_uiOscQuitPath)
    free(_uiOscQuitPath);
  _uiOscQuitPath = nullptr;
  if(_uiOscControlPath)
    free(_uiOscControlPath);
  _uiOscControlPath = nullptr;
  if(_uiOscConfigurePath)
    free(_uiOscConfigurePath);
  _uiOscConfigurePath = nullptr;
  if(_uiOscProgramPath)
    free(_uiOscProgramPath);
  _uiOscProgramPath = nullptr;
  if(_uiOscPath)
    free(_uiOscPath);
  _uiOscPath = nullptr;
}

//---------------------------------------------------------
//   oscShowGui
//---------------------------------------------------------

void OscIF::oscShowGui(bool v)
{
      #ifdef OSC_DEBUG
      fprintf(stderr, "OscIF::oscShowGui(): v:%d visible:%d\n", v, oscGuiVisible());
      #endif

      if (v == oscGuiVisible())
            return;

#ifdef _USE_QPROCESS_FOR_GUI_
      if(!_oscGuiQProc || (_oscGuiQProc->state() == QProcess::NotRunning))
#else
      if(_guiPid == -1)
#endif
      {
        // We need an indicator that update was called - update must have been called to get new path etc...
        // If the process is not running this path is invalid, right?
        if(_uiOscPath)
          free(_uiOscPath);
        _uiOscPath = nullptr;

        #ifdef OSC_DEBUG
        fprintf(stderr, "OscIF::oscShowGui(): No QProcess or process not running. Starting gui...\n");
        #endif

        if(!oscInitGui())
        {
          fprintf(stderr, "OscIF::oscShowGui(): failed to initialize gui on oscInitGui()\n");
          return;
        }
      }

      for (int i = 0; i < 10; ++i) {
            if (_uiOscPath)
                  break;
            sleep(1);
            }
      if (!_uiOscPath) {
            fprintf(stderr, "OscIF::oscShowGui(): no _uiOscPath. Error: Timeout - synth gui did not start within 10 seconds.\n");
            return;
            }

      lo_send(_uiOscTarget, v ? _uiOscShowPath : _uiOscHidePath, "");
      _oscGuiVisible = v;
}

//---------------------------------------------------------
//   oscGuiVisible
//---------------------------------------------------------

bool OscIF::oscGuiVisible() const
{
  return _oscGuiVisible;
}

//---------------------------------------------------------
//   oscQuitGui
//---------------------------------------------------------

bool OscIF::oscQuitGui()
{
      #ifdef OSC_DEBUG
      fprintf(stderr, "OscIF::oscQuitGui()\n");
      #endif

      if(!isRunning())
        return true;

      if(_uiOscTarget && _uiOscQuitPath)
      {
        #ifdef OSC_DEBUG
        fprintf(stderr, "OscIF::oscQuitGui(): Sending quit uiOscQuitPath:%s\n", _uiOscQuitPath);
        #endif
        lo_send(_uiOscTarget, _uiOscQuitPath, "");
      }
      else
        return false;

      if(!_oscGuiQProc->waitForFinished(10000)) // 10 secs.
      {
        fprintf(stderr, "OscIF::oscQuitGui(): Error: Timeout - Gui process is still running after 10 seconds!\n");
        return false;
      }

      // De-allocate all paths and stuff.
      oscCleanupGui();

      return true;
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
      //   DSSI_PROJECT_DIRECTORY_KEY, museProject.toUtf8().constData());  // MusEGlobal::song->projectPath()
      
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
  if(!_oscSynthIF || !_oscSynthIF->synthI() || !MusEGlobal::song)
    return false;

  int tidx = MusEGlobal::song->tracks()->index(_oscSynthIF->synthI());
  if(tidx == -1)
    return false;

  return OscIF::oscInitGui("dssi_synth",
                           _oscSynthIF->pluginLabel(),
                           tidx,
                           _oscSynthIF->dssiSynth()->fileName(),
                           _oscSynthIF->dssi_ui_filename(),
                           _oscSynthIF->dssiSynth()->getRpIdx());
}

QString OscDssiIF::displayName() const
{
  return _oscSynthIF ? _oscSynthIF->displayName() : QString();
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
  if(!_oscPluginI || !_oscPluginI->track() || !MusEGlobal::song)
    return false;

  int tidx = MusEGlobal::song->tracks()->index(_oscPluginI->track());
  if(tidx == -1)
    return false;

  return OscIF::oscInitGui("dssi_efx",
                           _oscPluginI->pluginLabel(),
                           tidx,
                           _oscPluginI->plugin()->fileName(),
                           _oscPluginI->dssi_ui_filename(),
                           _oscPluginI->plugin()->getRpIdx(),
                           _oscPluginI->id());
}

QString OscEffectIF::displayName() const
{
  return _oscPluginI ? _oscPluginI->displayName() : QString();
}


#else //OSC_SUPPORT
void initOSC() {}
void exitOSC() {}
#endif

} // namespace MusECore
