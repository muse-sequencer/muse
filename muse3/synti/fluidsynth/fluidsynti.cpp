//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./synti/fluidsynth/fluidsynti.cpp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//  (C) Copyright 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
/*
 * MusE FLUID Synth softsynth plugin
 *
 * Copyright (C) 2004 Mathias Lundgren (lunar_shuttle@users.sourcforge.net)
 *
 * $Id: fluidsynti.cpp,v 1.19.2.18 2009/12/06 10:05:00 terminator356 Exp $
 *
 */

#include <list>
#include <iostream>

#include <QFileInfo>
#include <QFileDialog>
#include <QString>
#include <QObject>
#include <QMutexLocker>
#include <QMessageBox>

//#include "common_defs.h"
#include "fluidsynti.h"
#include "muse/midi.h"


#ifdef HAVE_INSTPATCH
#include <libinstpatch/libinstpatch.h>
typedef std::multimap < int /* note */, std::string > NoteSampleNameList_t;
typedef NoteSampleNameList_t::iterator iNoteSampleNameList_t;
typedef NoteSampleNameList_t::const_iterator ciNoteSampleNameList_t;
typedef std::pair<int, std::string> NoteSampleNameInsertPair_t;
typedef NoteSampleNameList_t NoteSampleNameList;

typedef std::map < int /*patch*/, NoteSampleNameList_t > PatchNoteSampleNameList_t;
typedef PatchNoteSampleNameList_t::iterator iPatchNoteSampleNameList_t;
typedef PatchNoteSampleNameList_t::const_iterator ciPatchNoteSampleNameList_t;
typedef std::pair<iPatchNoteSampleNameList_t, bool> PatchNoteSampleNameListResult_t;
typedef std::pair<int, NoteSampleNameList_t> PatchNoteSampleNameInsertPair_t;
typedef PatchNoteSampleNameList_t PatchNoteSampleNameList;
#endif


FluidCtrl FluidSynth::fluidCtrl[] = {
      //{ "Expression", MusECore::CTRL_EXPRESSION, 0, 127 },
      //{ "Sustain", MusECore::CTRL_SUSTAIN, 0, 127 },
      //{ "Portamento", MusECore::CTRL_PORTAMENTO, 0, 127 },
      //{ "Soft Pedal", MusECore::CTRL_SOFT_PEDAL, 0, 127 },
      //{ "Variation", MusECore::CTRL_VARIATION_SEND, 0, 127 },
      //{ "Channel reverb send", MusECore::CTRL_REVERB_SEND, 0, 127 },
      //{ "Channel chorus send", MusECore::CTRL_CHORUS_SEND, 0, 127 },
      //{ "Pitch", MusECore::CTRL_PITCH, -8192, 8191 }
      
      // These controllers' initial values are set by the FS_PREDEF_ values, so just set them to zero here.
      { "Gain", FS_GAIN ,0, 127, 0},
      { "Master reverb on/off", FS_REVERB_ON , 0, 1, 0},
      { "Master reverb level", FS_REVERB_LEVEL, 0, 16384, 0},
      { "Master reverb size", FS_REVERB_ROOMSIZE, 0, 16384, 0}, // Interval: [0,1]
      { "Master reverb damping", FS_REVERB_DAMPING, 0, 16384, 0}, // Interval: [0,1]
      { "Master reverb width", FS_REVERB_WIDTH, 0, 16384, 0}, // Interval: [0,100]
      { "Master chorus on/off", FS_CHORUS_ON, 0, 1, 0},
      { "Master chorus num delay lines", FS_CHORUS_NUM, 0, 10, 0}, //Default: 3
      { "Master chorus type",  FS_CHORUS_TYPE, 0, 1, 0},
      { "Master chorus speed", FS_CHORUS_SPEED, 0, 16384, 0}, // (0.291,5) Hz
      { "Master chorus depth", FS_CHORUS_DEPTH, 0, 16384, 0}, // [0,40]
      { "Master chorus level", FS_CHORUS_LEVEL, 0, 16384, 0}, // [0,1]
      
      { "Program", MusECore::CTRL_PROGRAM, 0, 0xffffff, 0},
      { "Modulation", MusECore::CTRL_MODULATION, 0, 127, 0},
      { "Portamento time", MusECore::CTRL_PORTAMENTO_TIME, 0, 127, 0},
      { "Volume", MusECore::CTRL_VOLUME, 0, 127, 100},
      { "Pan", MusECore::CTRL_PANPOT, -64, 63, 0},
      { "Expression", MusECore::CTRL_EXPRESSION, 0, 127, 127},
      { "Sustain", MusECore::CTRL_SUSTAIN, 0, 127, 0},
      { "Portamento", MusECore::CTRL_PORTAMENTO, 0, 127, 0},
      { "Soft Pedal", MusECore::CTRL_SOFT_PEDAL, 0, 127, 0},
      { "Variation", MusECore::CTRL_VARIATION_SEND, 0, 127, 0},
      { "Channel reverb send", MusECore::CTRL_REVERB_SEND, 0, 127, 40},
      { "Channel chorus send", MusECore::CTRL_CHORUS_SEND, 0, 127, 0},
      { "Pitch", MusECore::CTRL_PITCH, -8192, 8191, 0},
      { "Channel pressure", MusECore::CTRL_AFTERTOUCH, 0, 127, 0},
      // Added by T356
      { "Pitch bend sensitivity", FS_PITCHWHEELSENS, 0, 24, 2}
    };

static int NUM_CONTROLLER = sizeof(FluidSynth::fluidCtrl)/sizeof(*(FluidSynth::fluidCtrl));

QString *projPathPtr;
//
// Fluidsynth
//
FluidSynth::FluidSynth(int sr, QMutex &_GlobalSfLoaderMutex) : Mess(2), _sfLoaderMutex(_GlobalSfLoaderMutex)
      {
      gui = 0;
      setSampleRate(sr);
      fluid_settings_t* s = new_fluid_settings();
      fluid_settings_setnum(s, (char*) "synth.sample-rate", float(sampleRate()));
      fluidsynth = new_fluid_synth(s);
      if (!fluidsynth) {
            printf("Error while creating fluidsynth!\n");
            return;
            }

      //Set up channels:
      for (int i=0; i<FS_MAX_NR_OF_CHANNELS; i++) {
            //channels[i].font       = 0;
            channels[i].font_extid = FS_UNSPECIFIED_ID;
            channels[i].font_intid = FS_UNSPECIFIED_ID;
            channels[i].preset     = FS_UNSPECIFIED_PRESET;
            channels[i].drumchannel= false;
      }

      initBuffer  = 0;
      initLen     = 0;

      QObject::connect(&fontWorker,SIGNAL(loadFontSignal(void*)),&fontWorker,SLOT(execLoadFont(void*)));
      fontWorker.moveToThread(&fontLoadThread);
      fontLoadThread.start();
      }

FluidSynth::~FluidSynth()
      {

      fontLoadThread.exit();

      for (std::list<FluidSoundFont>::iterator it =stack.begin(); it !=stack.end(); it++) 
      {
        if(it->intid == FS_UNSPECIFIED_FONT || it->intid == FS_UNSPECIFIED_ID) 
          continue;
        //Try to unload soundfont
        int err = fluid_synth_sfunload(fluidsynth, it->intid, 0);
        if(err == -1)  
          std::cerr << DEBUG_ARGS << "Error unloading soundfont!" << fluid_synth_error(fluidsynth) << std::endl;
      }
        
      int err = delete_fluid_synth (fluidsynth);
      if(gui)
        delete gui;

      if (initBuffer)
            delete [] initBuffer;
      if (err == -1) {
            std::cerr << DEBUG_ARGS << "error while destroying synth: " << fluid_synth_error(fluidsynth) << std::endl;
            return;
            }            
      }

bool FluidSynth::init(const char* name)
      {
      debug("FluidSynth::init\n");

      gui = new FluidSynthGui();
      gui->setWindowTitle(name);

      lastdir= "";
      currentlyLoadedFonts = 0;
      nrOfSoundfonts = 0;
      sendChannelData();
      cho_on = false;
      cho_num = FS_PREDEF_CHORUS_NUM;
      cho_type = FS_PREDEF_CHORUS_TYPE;
      cho_level = FS_PREDEF_CHORUS_LEVEL;
      cho_speed = FS_PREDEF_CHORUS_SPEED;
      cho_depth = FS_PREDEF_CHORUS_DEPTH;
      setController(0, FS_GAIN, (int)(fluidCtrl[0].max*FS_PREDEF_VOLUME));
      setController(0, FS_REVERB_ON, 0);
      setController(0, FS_REVERB_LEVEL, (int)(fluidCtrl[2].max*FS_PREDEF_REVERB_LEVEL));
      setController(0, FS_REVERB_ROOMSIZE, (int)(fluidCtrl[3].max*FS_PREDEF_REVERB_ROOMSIZE));
      setController(0, FS_REVERB_DAMPING, (int)(fluidCtrl[4].max*FS_PREDEF_REVERB_DAMPING));
      setController(0, FS_REVERB_WIDTH, (int)(fluidCtrl[5].max*FS_PREDEF_REVERB_WIDTH));
      setController(0, FS_CHORUS_ON, 0);
      setController(0, FS_CHORUS_NUM, FS_PREDEF_CHORUS_NUM);
      //setController(0, FS_CHORUS_TYPE, FS_PREDEF_CHORUS_TYPE); //?
      setController(0, FS_CHORUS_SPEED, (int)(fluidCtrl[9].max*FS_PREDEF_CHORUS_SPEED));
      setController(0, FS_CHORUS_DEPTH, (int)(fluidCtrl[10].max*FS_PREDEF_CHORUS_DEPTH));
      setController(0, FS_CHORUS_LEVEL, (int)(fluidCtrl[11].max*FS_PREDEF_CHORUS_LEVEL));
      return false;
      }

int FluidSynth::oldMidiStateHeader(const unsigned char** data) const 
{
  static unsigned char const d[2] = {MUSE_SYNTH_SYSEX_MFG_ID, FLUIDSYNTH_UNIQUE_ID};
  *data = &d[0];
  return 2; 
}
        
//---------------------------------------------------------
//   processMessages
//   Called from host always, even if output path is unconnected.
//---------------------------------------------------------

void FluidSynth::processMessages()
{
  //Process messages from the gui
  while (gui->fifoSize()) 
  {
    MusECore::MidiPlayEvent ev = gui->readEvent();
    if (ev.type() == MusECore::ME_SYSEX) 
    {
      sysex(ev.len(), ev.data());
      sendEvent(ev);
    }
    else if (ev.type() == MusECore::ME_CONTROLLER) 
    {
      setController(ev.channel(), ev.dataA(), ev.dataB(), true);
      sendEvent(ev);
    }
    else 
    {
      if (FS_DEBUG)
            printf("FluidSynth::processMessages(): unknown event, type: %d\n", ev.type());
    }
  }

}

//---------------------------------------------------------
//   process
//   Called from host, ONLY if output path is connected.
//---------------------------------------------------------

void FluidSynth::process(unsigned /*pos*/, float** ports, int offset, int len)
      {
      /*
      //Process messages from the gui
      while (gui->fifoSize()) {
            MusECore::MidiPlayEvent ev = gui->readEvent();
            if (ev.type() == MusECore::ME_SYSEX) {
                  sysex(ev.len(), ev.data());
                  sendEvent(ev);
                  }
            else if (ev.type() == MusECore::ME_CONTROLLER) {
                  setController(ev.channel(), ev.dataA(), ev.dataB(), true);
                  sendEvent(ev);
                  }
            else {
                  if (FS_DEBUG)
                        printf("FluidSynth::process(): unknown event, type: %d\n", ev.type());
                  }
            }
      */

      if (fluid_synth_write_float(fluidsynth, len, ports[0], offset, 1, ports[1], offset, 1)) {
            M_ERROR("Error writing from synth!");
            return;
            }
      }

//---------------------------------------------------------
//   getInitData
// Prepare data that will restore the synth's state on load
//---------------------------------------------------------
void FluidSynth::getInitData(int* n, const unsigned char** data) 
      {

 //printf("projPathPtr ");
 //std::cout << *projPathPtr << std::endl;

      // Data setup:
      // FS_INIT_DATA (1 byte)
      // FluidSynth version (2 bytes, x.y)
      // n = Number of soundfonts (1 byte)
      // Lastdir (variable size)
      //
      // FS_FONTS_BEGIN
      // n blocks with font path (variable size)
      // n bytes with font external id
      //
      // for all channels (16), 1 byte each for external id + 1 byte for preset + 1 byte for bankno
      // which is mapped to internal id after all fonts are loaded.
      //
      // reverb + chorus on/off (2 bytes)
      if (FS_DEBUG)
            printf("FluidSynth::getInitData()\n");

      //Calculate length:
      int len = FS_INIT_DATA_HEADER_SIZE + strlen(lastdir.c_str()) + 1; //header size
      for (std::list<FluidSoundFont>::const_iterator it = stack.begin(); it!=stack.end(); it++) {

            // if the soundfont is located under the projectPath we extract this from the filename
            int fileLen = it->file_name.size();
            if (it->file_name.startsWith(*projPathPtr)) {
                printf("project path found in filename, len %d shortened with %d\n",fileLen, projPathPtr->length()+1);
                fileLen = fileLen - projPathPtr->length()-1;
                }
            len+=fileLen + 2;
            }
      //Add length for lastdir and channels:
      len+=strlen(lastdir.c_str())+1;
      len+=(FS_MAX_NR_OF_CHANNELS*4); // 4 bytes: ext+int id + bankno + drumchannel status
      // + reverb
      len+=2;

      if (FS_DEBUG)
            printf("Total length of init sysex: %d\n", len);
      
      //byte* d = new byte[len];
      if (len > initLen) {
            if (initBuffer)
                  delete [] initBuffer;
            initBuffer = new byte[len];
            initLen = len;    
            }

      // Header:
      //d[0] = FS_INIT_DATA;
      //d[1] = FS_VERSION_MAJOR;
      //d[2] = FS_VERSION_MINOR;
      //d[3] = stack.size();
      initBuffer[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      initBuffer[1] = FLUIDSYNTH_UNIQUE_ID;
      initBuffer[2] = FS_INIT_DATA;
      initBuffer[3] = FS_VERSION_MAJOR;
      initBuffer[4] = FS_VERSION_MINOR;
      initBuffer[5] = stack.size();

      //Lastdir:
      byte* chptr = initBuffer + FS_INIT_DATA_HEADER_SIZE;
      memcpy(chptr, lastdir.c_str(), strlen(lastdir.c_str())+1);

      //For each font...
      chptr+=strlen(lastdir.c_str())+1;
      for (std::list<FluidSoundFont>::const_iterator it =stack.begin(); it!=stack.end(); it++) {

            // if the soundfont is located under the projectPath we extract this from the filename
            int offset=0;
            if (it->file_name.startsWith(*projPathPtr)) {
                offset= projPathPtr->length()+1;
            }

            memcpy(chptr, it->file_name.toLocal8Bit().data()+offset, it->file_name.size()-offset+1);
            //printf("path name stored=%s\n", it->filename.c_str()+offset);
            chptr = chptr + 1 + it->file_name.size()-offset;
            }

      //For each font again...
      *chptr = FS_INIT_CHANNEL_SECTION;
      chptr++;
      for (std::list<FluidSoundFont>::const_iterator it =stack.begin(); it!=stack.end(); it++) {
            *chptr = it->extid;
            chptr++;
            }

      //External id:s & preset for all channels:
      for(int i=0; i<FS_MAX_NR_OF_CHANNELS; i++) {
            *chptr = channels[i].font_extid; chptr++;
            *chptr = channels[i].preset; chptr++;
            *chptr = channels[i].banknum; chptr++;
            *chptr = channels[i].drumchannel; chptr++;
            }

      //Reverb:
      *chptr = rev_on; chptr++;
      *chptr = cho_on; chptr++;
      if (FS_DEBUG) {
            for (int i=0; i<len; i++)
                  printf("%c ", initBuffer[i]);
            printf("\n");
            for (int i=0; i<len; i++)
                  printf("%x ", initBuffer[i]);
            printf("\n");
            }
      // Give values to host:
      *data = (unsigned char*)initBuffer;
      *n = len;
      }

//-----------------------------------
// parseInitData
//-----------------------------------
void FluidSynth::parseInitData(int n, const byte* d)
{
      printf("projPathPtr ");
      std::cout << *projPathPtr->toLocal8Bit().data() << std::endl;

      bool load_drumchannels = true; // Introduced in initdata ver 0.3
      bool handle_bankvalue  = true; // Introduced in initdata ver 0.4

      if (FS_DEBUG) {
            printf("--- PARSING INIT DATA ---\n");
            for (int i=0; i<n; i++)
                  printf("%c ", d[i]);
            printf("\n");
            }

      byte version_major, version_minor;
      version_major = d[1]; version_minor = d[2];
      //version_major = d[3]; version_minor = d[4];

      // Check which version of the initdata we're using and if it's OK
      if (!(version_major == FS_VERSION_MAJOR && version_minor == FS_VERSION_MINOR)) {
            if (FS_DEBUG) {
                  printf("Project saved with other version of fluidsynth format. Ver: %d.%d\n", version_major, version_minor);
                  }

            if (version_major == 0 && version_minor == 1) {
              sendError("Initialization data created with different version of FluidSynth Mess, will be ignored.");
              return;
              }

            if (version_major == 0 && version_minor <= 2) {
                  load_drumchannels = false;
                  }

            if (version_major == 0 && version_minor <= 3) {
                  handle_bankvalue = false;
                  }
            }

      byte nr_of_fonts = d[3];
      //byte nr_of_fonts = d[5];
      nrOfSoundfonts = nr_of_fonts; //"Global" counter
      //byte* chptr = (unsigned char*)d + 4;
      int arrayIndex = 4;
      //const byte* chptr = (d + FS_INIT_DATA_HEADER_SIZE);

      //Get lastdir:
      lastdir = std::string((char*)&d[arrayIndex]);
      sendLastdir(lastdir.c_str());

      arrayIndex+=strlen(lastdir.c_str())+1;

      printf("Number of soundfonts for this instance: %d\n", nr_of_fonts);

      FluidSoundFont* fonts = new FluidSoundFont[nrOfSoundfonts]; //Just a temp one
      //Fonts:
      for (int i=0; i<nr_of_fonts; i++) {
            fonts[i].file_name = QString::fromLatin1((char*)&d[arrayIndex]);
            arrayIndex+=fonts[i].file_name.size()+1;
            QByteArray ba = projPathPtr->toLocal8Bit();

            if (QFileInfo(fonts[i].file_name).isRelative()) {
                printf("path is relative, we append full path!\n");
                fonts[i].file_name = QString(ba) + "/"+ fonts[i].file_name;
                }
            std::cout << "SOUNDFONT FILENAME + PATH " << fonts[i].file_name.toLocal8Bit().data() << std::endl;
            }

      if (d[arrayIndex] != FS_INIT_CHANNEL_SECTION) {
            delete[] fonts;
            sendError("Init-data corrupt... Projectfile error. Initdata ignored.\n");
            return;
            }

      arrayIndex++;
      for (int i=0; i<nr_of_fonts; i++) {
            fonts[i].extid = d[arrayIndex];
            arrayIndex++;
            //printf("Extid, %d: %d\n",i,fonts[i].extid);
            }

      // All channels external id + preset
      for (int i=0; i<FS_MAX_NR_OF_CHANNELS; i++) {
            channels[i].font_extid = d[arrayIndex]; arrayIndex++;
            channels[i].preset     = d[arrayIndex]; arrayIndex++;
            if (handle_bankvalue) { // Ver 0.4 and later
                  channels[i].banknum = d[arrayIndex]; arrayIndex++;
                  }
            else {
                  channels[i].banknum = 0;
                  }

            if (load_drumchannels) { // Ver 0.3 and later
                  channels[i].drumchannel = d[arrayIndex];
                  arrayIndex++;
                  }
            }

      //Reverb:
      setController(0, FS_REVERB_ON, d[arrayIndex]); arrayIndex++;
      setController(0, FS_CHORUS_ON, d[arrayIndex]); arrayIndex++;

      //if (FS_DEBUG)
            printf("--- END PARSE INIT DATA ---\n");


      for (int i=0; i<nrOfSoundfonts; i++) {
            pushSoundfont(fonts[i].file_name.toLocal8Bit().data(), fonts[i].extid);
            }
      delete[] fonts;
}


//---------------------------------------------------------
//   processEvent
//    All events from the sequencer goes here
//---------------------------------------------------------

bool FluidSynth::processEvent(const MusECore::MidiPlayEvent& ev)
      {
      switch(ev.type()) {
            case MusECore::ME_CONTROLLER:
                  if (FS_DEBUG_DATA) {
                        printf("*** FluidSynth::process - Controller. Chan: %x dataA: %x dataB: %x\n", ev.channel(), ev.dataA(), ev.dataB());
                        for (int i=0; i< ev.len(); i++)
                              printf("%x ", ev.data()[i]);
                        }
                  setController(ev.channel(), ev.dataA(), ev.dataB(), false);
                  //return true;  // ?? 
                  break;            
            case MusECore::ME_NOTEON:
                  return playNote(ev.channel(), ev.dataA(), ev.dataB());
            case MusECore::ME_NOTEOFF:
                  return playNote(ev.channel(), ev.dataA(), 0);
            case MusECore::ME_SYSEX:
                  //Debug print
                  if (FS_DEBUG_DATA) {
                        printf("*** FluidSynth::process - Sysex received\n");
                        for (int i=0; i< ev.len(); i++)
                              printf("%x ", ev.data()[i]);
                        printf("\n");
                        }
                  return sysex(ev.len(), ev.data());
            case MusECore::ME_PITCHBEND:
                setController(ev.channel(), MusECore::CTRL_PITCH, ev.dataA(), false);
                break;            
            case MusECore::ME_AFTERTOUCH:
                setController(ev.channel(), MusECore::CTRL_AFTERTOUCH, ev.dataA(), false);
                break;
            // Synths are not allowed to receive ME_PROGRAM, CTRL_HBANK, or CTRL_LBANK alone anymore - only CTRL_PROGRAM.
            //case MusECore::ME_PROGRAM:
            //    setController(ev.channel(), MusECore::CTRL_PROGRAM, ev.dataA(), false);
            break;   
            default:
            break;
      }
      return false;
      }

//---------------------------------------------------------
//   sysex
//---------------------------------------------------------

bool FluidSynth::sysex(int n, const unsigned char* d)
      {
      if(n < 3 || d[0] != MUSE_SYNTH_SYSEX_MFG_ID 
          || d[1] != FLUIDSYNTH_UNIQUE_ID) 
      {
        if (FS_DEBUG)
          printf("MusE FluidSynth: Unknown sysex header\n");
        return false;
      }
      
      //switch(*d) {
      const unsigned char* chrptr = d + 2;
      switch(*chrptr) {
            case FS_LASTDIR_CHANGE: {
                  lastdir = std::string((char*)(chrptr+1));
                  sendLastdir(lastdir.c_str());
                  break;
                  }
            case FS_PUSH_FONT: {
                  int extid = chrptr[1];

                  if (FS_DEBUG)
                        printf("Client: Got push font %s, id: %d\n",(chrptr+1), extid);

                  const char* filename = (const char*)(chrptr+2);
                  if (!pushSoundfont(filename, extid))
                              sendError("Could not load soundfont ");
                  break;
                  }
            case FS_DUMP_INFO: {
                  dumpInfo();
                  break;
                  }
            case FS_SOUNDFONT_CHANNEL_SET: {
                  sfChannelChange(*(chrptr+1), *(chrptr+2));
                  break;
                  }
            case FS_INIT_DATA: {
                  parseInitData(n - 2, chrptr);
                  break;
                  }
            case FS_SOUNDFONT_POP:
                  popSoundfont(*(chrptr+1));
                  break;
            case FS_DRUMCHANNEL_SET: {
                  byte onoff = (*(chrptr+1));
                  byte channel = (*(chrptr+2));
                  channels[channel].drumchannel = onoff;
                  if (FS_DEBUG)
                        printf("Client: Set drumchannel on chan %d to %d\n",channel, onoff);
                  break;
                  }
            default:
                  if (FS_DEBUG)
                        printf("FluidSynth::sysex() : unknown sysex received: %d\n",*chrptr);
                  break;
            }
      return false;
      }

//---------------------------------------------------------
//   sendSysex
//---------------------------------------------------------
void FluidSynth::sendSysex(int l, const unsigned char* d)
      {
      MusECore::MidiPlayEvent ev(0, 0, MusECore::ME_SYSEX, d, l);
      //printf("FluidSynth::sendSysex gui:%p\n", gui); 
      gui->writeEvent(ev);
      }

//-----------------------------------
// pushSoundfont - load a soundfont
//-----------------------------------
bool FluidSynth::pushSoundfont (const char* filename, int extid)
      {
      FS_Helper *helper = new FS_Helper;
      helper->fptr = this;
      helper->id = extid;
      QString fn = QString::fromLatin1(filename);

      if (QFile::exists(fn))
      {
              helper->file_name = fn;
      }
      else
      {

          //printf("current path: %s \nmuseProject %s\nfilename %s\n",QDir::currentPath().toLocal8Bit().data(), MusEGlobal::museProject.toLocal8Bit().data(), filename);
          QFileInfo fi(fn);
          if (QFile::exists(fi.fileName())) // check if the file exists in current folder
              helper->file_name = QDir::currentPath() + "/" + fi.fileName();
          else {
              // TODO: Strings should be translated, this does
              //       however require the class to be derived from qobject
              //       tried in vain to make the call in the gui object
              //       could'nt get it to work due to symbol missing in .so ...
//              helper->file_name = QFileDialog::getOpenFileName(0,
//                                      QString("Can't find soundfont: %1 - Choose soundfont").arg(filename),
//                                      fn,
//                                      QString("Soundfonts (*.sf2);;All files (*)"));
//              QMessageBox::warning(NULL,"No sound font found.","Could not open soundfont: " + fn, QMessageBox::Ok);

              fprintf(stderr, "Warning: Could not open soundfont: %s\n", fn.toLocal8Bit().data());

              return false;
          }
      }

      fontWorker.loadFont(helper);

      return true;
      }


#ifdef HAVE_INSTPATCH
static void loadSf2NoteSampleNames(FluidSoundFont& font, IpatchSF2 *sf2)
{
  IpatchList *presets, *pZones, *iZones;
  IpatchIter pIter, pZoneIter, iZoneIter;
  IpatchItem *pset, *pZone, *inst, *iZone;
  char *psetName, *instName, *sampName;
  int bank, program, patch;
  gboolean pRangeSet, iRangeSet;
  IpatchSF2GenAmount pNoteRange, iNoteRange;
  IpatchSF2Sample *samp;

  // Get preset children of SoundFont
  presets = ipatch_container_get_children (IPATCH_CONTAINER (sf2), IPATCH_TYPE_SF2_PRESET);
  ipatch_list_init_iter (presets, &pIter);

  // Iterate over presets in list
  for (pset = ipatch_item_first (&pIter); pset != NULL; pset = ipatch_item_next (&pIter))
  { // Get name of preset, MIDI bank, and MIDI program number
    g_object_get (pset,
                  "name", &psetName,
                  "bank", &bank,
                  "program", &program,
                  NULL);


    //fprintf(stderr, "psetName:%s bank:%d program:%d\n", psetName, bank, program);

    // Compose a patch numer. Drums are on special bank 128.
    patch = (bank << 16) | (0xff << 8) | (program & 0x7f);
    PatchNoteSampleNameListResult_t res_pnsnl = font._noteSampleNameList.insert(PatchNoteSampleNameInsertPair_t(patch, NoteSampleNameList()));
    iPatchNoteSampleNameList_t res_ipnsnl = res_pnsnl.first;
    NoteSampleNameList& nsl = res_ipnsnl->second;

    // Get preset zones
    pZones = ipatch_container_get_children (IPATCH_CONTAINER (pset), IPATCH_TYPE_SF2_ZONE);
    ipatch_list_init_iter (pZones, &pZoneIter);

    // Iterate over preset zones
    for (pZone = ipatch_item_first (&pZoneIter); pZone != NULL; pZone = ipatch_item_next (&pZoneIter))
    { // Get linked instrument and preset zone note range set flag
      g_object_get (pZone,
                    "link-item", &inst,
                    "note-range-set", &pRangeSet,
                    NULL);

      // Get instrument name and global zone note range
      g_object_get (inst,
                    "name", &instName,
                    NULL);

      //fprintf(stderr, "\tinstName:%s\n", instName);

      // Get instrument zones
      iZones = ipatch_container_get_children (IPATCH_CONTAINER (inst), IPATCH_TYPE_SF2_ZONE);
      ipatch_list_init_iter (iZones, &iZoneIter);

      for (iZone = ipatch_item_first (&iZoneIter); iZone != NULL; iZone = ipatch_item_next (&iZoneIter))
      { // Get sample and instrument zone note range set flag
        g_object_get (iZone,
                      "note-range-set", &iRangeSet,
                      "link-item", &samp,
                      NULL);

        // Get instrument name and global zone note range
        g_object_get (samp,
                      "name", &sampName,
                      NULL);

        if (pRangeSet)
          ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (pZone), IPATCH_SF2_GEN_NOTE_RANGE, &pNoteRange);
        else ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (pset), IPATCH_SF2_GEN_NOTE_RANGE, &pNoteRange);

        if (iRangeSet)
          ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (iZone), IPATCH_SF2_GEN_NOTE_RANGE, &iNoteRange);
        else ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (inst), IPATCH_SF2_GEN_NOTE_RANGE, &iNoteRange);

        if (ipatch_sf2_gen_range_intersect (&iNoteRange, &pNoteRange))    // Returns false if no common range
        {
          // Note range spans iNoteRange.range.low to iNoteRange.range.high
          //fprintf(stderr, "\t\t%s\tiNoteRange h:%u l:%u  pNoteRange h:%u l:%u\n", sampName, iNoteRange.range.high, iNoteRange.range.low, pNoteRange.range.high, pNoteRange.range.low);

          for(int iNote = iNoteRange.range.low; iNote <= iNoteRange.range.high; ++iNote) // Yes, that's less than or equal.
            nsl.insert(NoteSampleNameInsertPair_t(iNote, std::string(sampName)));
        }

        g_free (sampName);
        g_object_unref (samp);
      }

      g_object_unref (iZones);
      g_free (instName);
      g_object_unref (inst);
    }

    g_object_unref (pZones);
    g_free (psetName);
  }

  g_object_unref (presets);
}

//---------------------------------------------------------
//   loadNoteSampleNames
//    Extracts all the note sample names
//---------------------------------------------------------

static void loadNoteSampleNames(FluidSoundFont& font)
{
  IpatchSF2 *sf2;
  IpatchSF2Reader* sf2_reader;
  IpatchFileHandle *fhandle;
  IpatchSF2File *sffile;
  GError *err = NULL;
  const QByteArray ba = font.file_name.toLocal8Bit();
  const char* fname = ba.constData();

  /* initialize libInstPatch */
  ipatch_init ();

  sffile = ipatch_sf2_file_new ();

  fhandle = ipatch_file_open(IPATCH_FILE(sffile), fname, "r", &err);
  if(!fhandle)
  {
    fprintf (stderr, "Failed to identify file '%s': %s\n", fname,
              ipatch_gerror_message (err));
    g_clear_error (&err);
    g_object_unref (sffile);
    return;
  }

  sf2_reader = ipatch_sf2_reader_new(fhandle);
  sf2 = ipatch_sf2_reader_load(sf2_reader, NULL);

  loadSf2NoteSampleNames(font, sf2);

  ipatch_file_close(fhandle);
  g_object_unref (sf2);
  //g_object_unref(sf2_reader);  // Needed?
  g_object_unref (sffile);
}
#endif

//---------------------------------------------------------
//   playNote
//    called from host
//---------------------------------------------------------

bool FluidSynth::playNote(int channel, int pitch, int velo)
      {
      if (channels[channel].font_intid == FS_UNSPECIFIED_FONT ||
          channels[channel].font_intid == FS_UNSPECIFIED_ID)
            return false;
      
      if (velo) {
            if (fluid_synth_noteon(fluidsynth, channel, pitch, velo)) {
                  if (FS_DEBUG)
                        std::cerr << DEBUG_ARGS << "error processing noteon event: " << fluid_synth_error(fluidsynth);
                  }
            }
      else {
            if (fluid_synth_noteoff(fluidsynth, channel, pitch))
                  if (FS_DEBUG)
                        std::cerr << DEBUG_ARGS << "error processing noteoff event: " << fluid_synth_error(fluidsynth) << std::endl;
            }
      return false;
      }
//---------------------------------------------------------
//   sendSoundFontData
//---------------------------------------------------------
void FluidSynth::sendSoundFontData()
      {
      int ndatalen = 2; //2 bytes for command and length
      //int ndatalen = 4; // 4 bytes for header, command and length

      //Calculate length in chars of all strings in the soundfontstack in one string
      for (std::list<FluidSoundFont>::iterator it = stack.begin(); it != stack.end(); it++) {
            ndatalen += 1 + it->name.size();
            ndatalen += FS_SFDATALEN; //unsigned char for ID
            }
      byte ndata[ndatalen];
      *(ndata) = FS_SEND_SOUNDFONTDATA; //The command
      *(ndata + 1) = (unsigned char)stack.size (); //Nr of Soundfonts
      //*ndata = MUSE_SYNTH_SYSEX_MFG_ID; 
      //*(ndata + 1) = FLUIDSYNTH_UNIQUE_ID; 
      //*(ndata + 2) = FS_SEND_SOUNDFONTDATA; //The command
      //*(ndata + 3) = (unsigned char)stack.size (); //Nr of Soundfonts

      // Copy the stuff to ndatalen:
      char* chunk_start = (char*)(ndata + 2);
      //char* chunk_start = (char*)(ndata + 4);
      int chunk_len, name_len;
      for (std::list<FluidSoundFont>::iterator it = stack.begin(); it != stack.end();  ++it) {
            name_len = it->name.size() + 1;
            chunk_len = name_len + FS_SFDATALEN;
            memcpy(chunk_start, it->name.toLocal8Bit().data(), name_len); //First, store the fontname
            *(chunk_start + name_len) = it->extid; //The GUI only needs to know about the external id, store that here
            chunk_start += chunk_len;
            }
      sendSysex(ndatalen, ndata);
      }

//---------------------------------------------------------
//   sendChannelData
//---------------------------------------------------------
void FluidSynth::sendChannelData()
      {
      ///int chunk_size = 2;
      int const chunk_size = 2;
      int chdata_length = (chunk_size * FS_MAX_NR_OF_CHANNELS) +1 ; //Command and the 2 channels * 16
      //int chdata_length = (chunk_size * FS_MAX_NR_OF_CHANNELS) +3 ; // Header, command and the 2 channels * 16
      byte chdata[chdata_length];
      byte* chdptr;
      chdata[0] = FS_SEND_CHANNELINFO;
      //chdata[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      //chdata[1] = FLUIDSYNTH_UNIQUE_ID;
      //chdata[2] = FS_SEND_CHANNELINFO;
      chdptr = (chdata + 1);
      //chdptr = (chdata + 3);
      for (int i=0; i<FS_MAX_NR_OF_CHANNELS; i++) {
            *(chdptr)    = channels[i].font_extid; //Font external id
            *(chdptr+1)  = i; //Channel nr
            chdptr += chunk_size;
            }
      sendSysex(chdata_length, chdata);
      // Send drum channel info afterwards (later addition, not very neat, but works...)

      int drumchdata_length = FS_MAX_NR_OF_CHANNELS + 1; //1 byte for the command, one byte for each channel
      //int drumchdata_length = FS_MAX_NR_OF_CHANNELS + 3; // 2 bytes for header, 1 byte for the command, one byte for each channel
      byte drumchdata[drumchdata_length];
      *drumchdata = FS_SEND_DRUMCHANNELINFO;
      //drumchdata[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      //drumchdata[1] = FLUIDSYNTH_UNIQUE_ID;
      //drumchdata[2] = FS_SEND_DRUMCHANNELINFO;
      
      byte* drumchdataptr = drumchdata;
      //byte* drumchdataptr = drumchdata + 3;
      
      for (int i=0; i<FS_MAX_NR_OF_CHANNELS; i++) {
            drumchdataptr++;
            *drumchdataptr = channels[i].drumchannel;
            }
      // FIXME By Tim. This is crashing, after the conversion to QT4 and cmake.            
      //usleep(1000);
      sendSysex(drumchdata_length, drumchdata);
      }

//---------------------------------------------------------
//   dumpInfo
//---------------------------------------------------------

void FluidSynth::dumpInfo()
      {
      printf("-----------------------------------------------------\n");
      printf("Dumping info...\n");
      printf("Last dir: %s\n", lastdir.c_str());
      for (int i=0; i<FS_MAX_NR_OF_CHANNELS; i++)
            printf("Chan %d\tFont extid:%d\tintid:%d\tdrumchan:%d\tpreset: %d\n", i, channels[i].font_extid, channels[i].font_intid, channels[i].drumchannel, channels[i].preset);

      printf("\n");
      for (std::list<FluidSoundFont>::iterator it = stack.begin(); it != stack.end(); it++)
            printf("Font: %s\tintid: %d\textid %d\tfilename:%s\n", it->name.toLocal8Bit().data(), it->intid, it->extid, it->file_name.toLocal8Bit().data());
      printf("Reverb on: %d, width: %f, size: %f level: %f damp: %f\n",rev_on, rev_width, rev_size, rev_level, rev_damping);
      printf("-----------------------------------------------------\n");
      }

//---------------------------------------------------------
//   nativeGuiVisible
//---------------------------------------------------------

bool FluidSynth::nativeGuiVisible() const
      {
      return gui->isVisible();
      }


//---------------------------------------------------------
//   showNativeGui
//---------------------------------------------------------

void FluidSynth::showNativeGui(bool val)
      {
      gui->setVisible(val);
      }

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

bool FluidSynth::setController(int channel, int id, int val)
      {
      setController(channel, id, val, false);
      return false;
      }

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

void FluidSynth::setController(int channel, int id, int val, bool fromGui)
      {
      //
      // Channelless controllers
      //
      int err = 0;
      switch (id) {
            case FS_GAIN: {
                  fluid_synth_set_gain(fluidsynth, (float) val/25); //gives val an interval of approximately[0,5]
                  //Forward to gui if not from Gui
                  if (!fromGui) {
                        MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, FS_GAIN, val);
                        gui->writeEvent(ev);
                        }
                  break;
                  }
            case FS_REVERB_ON: {
                  rev_on = val;
                  fluid_synth_set_reverb_on(fluidsynth, val); // 0 or 1
                  //if (rev_on)
                  //      fluid_synth_set_reverb(fluidsynth, rev_size, rev_damping, rev_width, rev_level);
                  if (!fromGui) {
                        MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, FS_REVERB_ON, val);
                        gui->writeEvent(ev);
                        }
                  break;
                  }
            case FS_REVERB_LEVEL:
                  //Interval: 0-2
                  rev_level = (double)2*val/16384; //[0,2]
                  //if (rev_on)
                        fluid_synth_set_reverb(fluidsynth, rev_size, rev_damping, rev_width, rev_level);
                  if (!fromGui) {
                        MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, FS_REVERB_LEVEL, val);
                        gui->writeEvent(ev);
                        }
                  break;
            case FS_REVERB_WIDTH: //
                  rev_width = (double)val/164; //[0,100]
                  //if (rev_on)
                        fluid_synth_set_reverb(fluidsynth, rev_size, rev_damping, rev_width, rev_level);
                  if (!fromGui) {
                        MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, FS_REVERB_WIDTH, val);
                        gui->writeEvent(ev);
                        }
                  break;
            case FS_REVERB_DAMPING: //[0,1]
                  rev_damping = (double)val/16384;
                  //if (rev_on)
                        fluid_synth_set_reverb(fluidsynth, rev_size, rev_damping, rev_width, rev_level);
                  if (!fromGui) {
                        MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, FS_REVERB_DAMPING, val);
                        gui->writeEvent(ev);
                        }
                  break;
            case FS_REVERB_ROOMSIZE: //[0,1]
                  rev_size = (double)val/16384;
                  //if (rev_on)
                        fluid_synth_set_reverb(fluidsynth, rev_size, rev_damping, rev_width, rev_level);
                  if (!fromGui) {
                        MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, FS_REVERB_ROOMSIZE, val);
                        gui->writeEvent(ev);
                        }
                  break;
            case FS_CHORUS_ON: {// 0 or 1
                  cho_on = val;
                  fluid_synth_set_chorus_on(fluidsynth, val);
                  if (!fromGui) {
                        MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, FS_CHORUS_ON, val);
                        gui->writeEvent(ev);
                        }
                  break;
                  }
            case FS_CHORUS_NUM: {//Number of delay lines
                  cho_num = val;
                  fluid_synth_set_chorus(fluidsynth, cho_num, cho_level, cho_speed, cho_depth, cho_type);
                  if (!fromGui) {
                        MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, FS_CHORUS_NUM, val);
                        gui->writeEvent(ev);
                        }
                  break;
                  }
            case FS_CHORUS_TYPE: {//?
                  cho_type = val;
                  fluid_synth_set_chorus(fluidsynth, cho_num, cho_level, cho_speed, cho_depth, cho_type);
                  if (!fromGui) {
                        MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, FS_CHORUS_TYPE, val);
                        gui->writeEvent(ev);
                        }
                  break;
                  }
            case FS_CHORUS_SPEED: {//(0.291,5) Hz
                  cho_speed = (double)(0.291 + (double)val/3479);
                  fluid_synth_set_chorus(fluidsynth, cho_num, cho_level, cho_speed, cho_depth, cho_type);
                  if (!fromGui) {
                        MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, FS_CHORUS_SPEED, val);
                        gui->writeEvent(ev);
                        }
                  break;
                  }
            case FS_CHORUS_DEPTH: { //[0,40]
                  cho_depth = (double) val*40/16383;
                  fluid_synth_set_chorus(fluidsynth, cho_num, cho_level, cho_speed, cho_depth, cho_type);
                  if (!fromGui) {
                        MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, FS_CHORUS_DEPTH, val);
                        gui->writeEvent(ev);
                        }
                  break;
                  }
            case FS_CHORUS_LEVEL: { //[0,1]
                  cho_level = (double) val/16383;
                  fluid_synth_set_chorus(fluidsynth, cho_num, cho_level, cho_speed, cho_depth, cho_type);
                  if (!fromGui) {
                        MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, FS_CHORUS_LEVEL, val);
                        gui->writeEvent(ev);
                        }
                  break;
                  }
            //
            // Controllers that depend on channels
            //
            case MusECore::CTRL_PITCH:
                  // MusE's range is from -8192 to +8191, fluidsynth seems to be [0, 16384]
                  if(val != MusECore::CTRL_VAL_UNKNOWN)
                  {
                    val +=8192;
                    err = fluid_synth_pitch_bend (fluidsynth, channel, val);
                  }
                  break;
            case MusECore::CTRL_AFTERTOUCH:
                  if(val != MusECore::CTRL_VAL_UNKNOWN)
                    err = fluid_synth_channel_pressure (fluidsynth, channel, val);
                  break;
            case FS_PITCHWHEELSENS:
                  if(val != MusECore::CTRL_VAL_UNKNOWN)
                    err = fluid_synth_pitch_wheel_sens(fluidsynth, channel, val);
                  break;
            case MusECore::CTRL_PROGRAM: {
                  //Check if MusE is trying to set a preset on an unspecified font. If so, ignore.
                  if (FS_DEBUG)
                        printf("Program select : channel %d val %d\n",channel, val);
                  byte font_intid = channels[channel].font_intid;

                  if (font_intid == FS_UNSPECIFIED_ID || font_intid == FS_UNSPECIFIED_FONT)
                        return;

                  byte banknum = ((val >> 16) & 0xff);
                  byte patch = (val & 0xff);
                  //printf("val: %d banknum: %x patch: %d\n", val, banknum, patch);

                  if(val == MusECore::CTRL_VAL_UNKNOWN || patch == 0xff)
                        return;
                  if(channels[channel].drumchannel)
                    banknum = 128;
                  else if(banknum == 0xff)
                    banknum = 0; // Is wise? Else try to keep a previous value when 'off' (0xff) like the HW values?
                  
                  err = fluid_synth_program_select(fluidsynth, channel, font_intid , banknum, patch);
                  if (err)
                        printf("FluidSynth::setController() - Error changing program on soundfont %s, channel: %d\n", fluid_synth_error(fluidsynth), channel);
                  else {
                        channels[channel].preset = val;//setChannelPreset(val, channel);
                        channels[channel].banknum = banknum;
                        }
                  break;
                  }
            default:
                  if (FS_DEBUG)
                        printf("Setting controller on channel: %d with id: 0x%x to val: %d\n",channel, id, val);
                  err = fluid_synth_cc(fluidsynth, channel, id, val);
                  break;
            }

      if (err)
            printf ("FluidSynth::setController() - error processing controller event: %s\n", fluid_synth_error(fluidsynth));
      }

//---------------------------------------------------------
//   getControllerInfo
//---------------------------------------------------------
int FluidSynth::getControllerInfo(int id, QString* name, int* controller, int* min, int* max, int* initval) const
      {
      if (id >= NUM_CONTROLLER)
            return 0;
      *controller = fluidCtrl[id].num;
      *name       = QString(fluidCtrl[id].name);
      *min        = fluidCtrl[id].min;
      *max        = fluidCtrl[id].max;
      switch(id)
      {
        case 0:
          *initval = (int)(fluidCtrl[0].max*FS_PREDEF_VOLUME);
        break;
        case 1:
          *initval = 0;
        break;
        case 2:
          *initval = (int)(fluidCtrl[2].max*FS_PREDEF_REVERB_LEVEL);
        break;
        case 3:
          *initval = (int)(fluidCtrl[3].max*FS_PREDEF_REVERB_ROOMSIZE);
        break;
        case 4:
          *initval = (int)(fluidCtrl[4].max*FS_PREDEF_REVERB_DAMPING);
        break;
        case 5:
          *initval = (int)(fluidCtrl[5].max*FS_PREDEF_REVERB_WIDTH);
        break;
        case 6:
          *initval = 0;
        break;
        case 7:
          *initval = (int)(fluidCtrl[7].max*FS_PREDEF_CHORUS_NUM);
        break;
        case 8:
          *initval = (int)(fluidCtrl[8].max*FS_PREDEF_CHORUS_TYPE);
        break;
        case 9:
          *initval = (int)(fluidCtrl[9].max*FS_PREDEF_CHORUS_SPEED);
        break;
        case 10:
          *initval = (int)(fluidCtrl[10].max*FS_PREDEF_CHORUS_DEPTH);
        break;
        case 11:
          *initval = (int)(fluidCtrl[11].max*FS_PREDEF_CHORUS_LEVEL);
        break;
        default:      
          *initval = fluidCtrl[id].initval;
        break;  
      }  

      if (FS_DEBUG)
            printf("FluidSynth::getControllerInfo() id: %d name: %s controller: %d min: %d max: %d initval: %d\n",
                   id,(*name).toLocal8Bit().constData(),*controller,*min,*max,*initval);
      return ++id;
      }

#ifdef HAVE_INSTPATCH
bool FluidSynth::getNoteSampleName(bool drum, int channel, int patch, int note, QString* name) const
{
  if(!name || channel < 0 || channel >= FS_MAX_NR_OF_CHANNELS)
    return false;
  const FluidChannel& fc = channels[channel];
  if(fc.drumchannel != drum)
    return false;
  // Force the low bank to don't care, we don't use it in fluidsynth MESS).
  patch |= 0xff00;
  if(drum)
  {
    patch &= 0xffff;    // Remove the high bank.
    patch |= 0x800000;  // Set high bank to 128 (special soundfont bank number meaning drums), and low bank to don't care.
  }

  for(std::list<FluidSoundFont>::const_iterator it = stack.begin(); it != stack.end(); it++)
  {
    const FluidSoundFont& fsf = *it;
    if(fsf.intid == fc.font_intid) // || fsf.extid == fc.font_extid)
    {
      ciPatchNoteSampleNameList_t ipnsnl = fsf._noteSampleNameList.find(patch);
      if(ipnsnl != fsf._noteSampleNameList.end())
      {
        const NoteSampleNameList& pnsnl = ipnsnl->second;
        ciNoteSampleNameList_t insnl = pnsnl.find(note);
        if(insnl != pnsnl.end())
        {
          const std::string& str = insnl->second;
          *name = QString::fromStdString(str);
          return true;
        }
      }
    }
  }

  return false;
}
#endif

//---------------------------------------------------------
//   sendError
//---------------------------------------------------------
void FluidSynth::sendError(const char *errorMessage)
      {
      int len = 2 + strlen(errorMessage);
      //int len = 4 + strlen(errorMessage);
      unsigned char data[len];
      *data = FS_ERROR;
      //data[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      //data[1] = FLUIDSYNTH_UNIQUE_ID;
      //data[2] = FS_ERROR;
      memcpy(data + 1, errorMessage, len - 1);
      //memcpy(data + 3, errorMessage, len - 3);
      sendSysex(len, data);
      }

//---------------------------------------------------------
//   getNextAvailableExternalId
//---------------------------------------------------------

int FluidSynth::getNextAvailableExternalId()
      {
      unsigned char place[FS_MAX_NR_OF_CHANNELS];
      for(int i=0; i<FS_MAX_NR_OF_CHANNELS; i++)
            place[i] = 0;
      for (std::list<FluidSoundFont>::iterator it = stack.begin(); it != stack.end(); it++)
            place[it->extid] = 1;

      int i=0;
      while (i < FS_MAX_NR_OF_CHANNELS && place[i] == 1)
            i++;

      return i;
      }

//---------------------------------------------------------
//   sfChannelChange
//---------------------------------------------------------

void FluidSynth::sfChannelChange(byte extid, byte channel)
      {
      if (FS_DEBUG)
            printf("FluidSynth::sfChannelChange()-Setting channel %d to font with extid %d intid %d\n",channel, extid, getFontInternalIdByExtId(extid));
      channels[channel].font_extid = extid;
      channels[channel].font_intid = getFontInternalIdByExtId(extid);
      }

//---------------------------------------------------------
//   getFontInternalIdByExtId
//---------------------------------------------------------
byte FluidSynth::getFontInternalIdByExtId(byte ext_id)
      {
      for (std::list<FluidSoundFont>::iterator it = stack.begin(); it !=stack.end(); it++) {
            if (it->extid == ext_id)
                  return it->intid;
            }
      return FS_UNSPECIFIED_FONT;
      }

//---------------------------------------------------------
//   sendLastDir
//---------------------------------------------------------
void FluidSynth::sendLastdir(const char* lastdir)
      {
      int n = strlen(lastdir) + 2;
      //int n = strlen(lastdir) + 4;
      byte d[n];
      d[0] = FS_LASTDIR_CHANGE;
      //d[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      //d[1] = FLUIDSYNTH_UNIQUE_ID;
      //d[2] = FS_LASTDIR_CHANGE;
      memcpy(d+1,lastdir, strlen(lastdir)+1);
      //memcpy(d+3,lastdir, strlen(lastdir)+1);

      MusECore::MidiPlayEvent ev(0,0, MusECore::ME_SYSEX, d, n);
      gui->writeEvent(ev);
      }


//---------------------------------------------------------
//   rewriteChannelSettings
//---------------------------------------------------------
void FluidSynth::rewriteChannelSettings()
      {
      //Walk through the channels, remap internal ID:s to external ID:s (something that actually only needs to be done at
      //startup, since the fonts aren't loaded yet at that time and it isn't possible to give them a correct internal id
      //since they don't have any at that time, this can probably be fixed in a smarter way (but it works..))
      for (int i=0; i<FS_MAX_NR_OF_CHANNELS; i++) {
            int ext_id = channels[i].font_extid;//getFontExternalIdByChannel(i);
            if (ext_id != FS_UNSPECIFIED_ID) //Check if ext_id is set to any sane font
                  {
                  channels[i].font_intid = getFontInternalIdByExtId(ext_id);//(getFontInternalIdByExtId(ext_id));//if so, get value from the stack
                  }
            else
                  channels[i].font_intid = FS_UNSPECIFIED_FONT; //if not, set it to unspecified
            }

      //Assign correct presets to all channels
      for (int i=0; i<FS_MAX_NR_OF_CHANNELS; i++) {
            int preset = channels[i].preset;
            int int_id = channels[i].font_intid;
            byte banknum = channels[i].banknum;

            if (channels[i].drumchannel)
                  banknum = 128;

            //printf("Channel %d, font int-id %d ext_id %d, preset %d\n",i, int_id, getFontExternalIdByChannel(i), preset);
            if (!(preset == FS_UNSPECIFIED_PRESET 
                  || int_id == FS_UNSPECIFIED_FONT
                  || int_id == FS_UNSPECIFIED_ID)) {
                  int rv = fluid_synth_program_select(fluidsynth, i, int_id, banknum, preset);
                  if (rv)
                        std::cerr << DEBUG_ARGS << "Error changing preset! " << fluid_synth_error(fluidsynth) << std::endl;
                  }
            }
      }
//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------
QString FluidSynth::getPatchName(int i, int, bool /*drum*/) const
      {
      if (channels[i].font_intid == FS_UNSPECIFIED_FONT ||
          channels[i].font_intid == FS_UNSPECIFIED_ID)
            return "<unknown>";
      else if (channels[i].preset == FS_UNSPECIFIED_PRESET)
            return "<unknown>";
      else {
            fluid_preset_t *preset = fluid_synth_get_channel_preset(fluidsynth, i);
            if (!preset) return "<unknown>";
            return preset->get_name(preset);
            }
      }
//---------------------------------------------------------
//   getPatchInfo
//---------------------------------------------------------
const MidiPatch* FluidSynth::getPatchInfo(int i, const MidiPatch* patch) const
      {
      //if (channels[i].font_intid == FS_UNSPECIFIED_FONT)
      if (channels[i].font_intid == FS_UNSPECIFIED_FONT ||
          channels[i].font_intid == FS_UNSPECIFIED_ID)
            return 0;
      //else if (channels[i].preset == FS_UNSPECIFIED_PRESET)
      //      return 0;
      else {
            //printf("Getpatchname, channel: %d\n",channel);
            if (!patch)
                  //Deliver first patch
                  return getFirstPatch(i);
            else
                  //Deliver next patch
                  return getNextPatch(i, patch);
            }
      }

//---------------------------------------------------------
//   getFirstPatch
//---------------------------------------------------------
const MidiPatch* FluidSynth::getFirstPatch (int channel) const
      {
      static MidiPatch midiPatch;

      midiPatch.typ = 0;
      midiPatch.lbank = 0;

      fluid_preset_t* preset;
      int font_id = channels[channel].font_intid;
      //if (font_id == FS_UNSPECIFIED_FONT)
      if (font_id == FS_UNSPECIFIED_FONT || font_id == FS_UNSPECIFIED_ID)
            return 0;

      fluid_sfont_t* sfont = fluid_synth_get_sfont_by_id(fluidsynth, font_id);

      if (!channels[channel].drumchannel) {
            for (unsigned bank = 0; bank < 128; ++bank) {
                  for (unsigned patch = 0; patch < 128; ++patch) {
                        preset = sfont->get_preset (sfont, bank, patch);
                        if (preset) {
                              midiPatch.hbank = bank;
                              midiPatch.lbank = 0xff;  // Off
                              midiPatch.prog = patch;
                              midiPatch.name = preset->get_name (preset);
                              return &midiPatch;
                              }
                        }
                  }
            return 0;
            }
      else { //This is a drumchannel
            int bank = 128;
            for (unsigned patch = 0; patch < 128; ++patch) {
                  preset = sfont->get_preset (sfont, bank, patch);
                  if (preset) {
                        midiPatch.hbank = 0xff;  // Off
                        midiPatch.lbank = 0xff;  // Off
                        midiPatch.prog = patch;
                        midiPatch.name = preset->get_name(preset);
                        return &midiPatch;
                        }
                  }
            }
      return 0;
      }

//---------------------------------------------------------
//   getNextPatch
//---------------------------------------------------------
const MidiPatch* FluidSynth::getNextPatch (int channel, const MidiPatch* patch) const
      {
      static MidiPatch midiPatch;
      //First check if there actually is any soundfont associated to the channel. If not, don't bother
      int font_id = channels[channel].font_intid;
      if (font_id == FS_UNSPECIFIED_FONT || font_id == FS_UNSPECIFIED_ID)
            return 0;
      if (patch == 0)
            return getFirstPatch(channel);
      midiPatch.typ = 0;
      midiPatch.lbank = 0;

      //printf("Font has internal id: %d\n",font_id);
      fluid_preset_t* preset;
      fluid_sfont_t* sfont = fluid_synth_get_sfont_by_id(fluidsynth, font_id);

      if (!channels[channel].drumchannel) {
            unsigned prog = patch->prog + 1;

            for (unsigned bank = patch->hbank; bank < 128; ++bank) {
                  for ( ; prog < 128; ++prog) {
                        preset = sfont->get_preset (sfont, bank, prog);
                        if (preset) {
                              //printf("Preset info: bank: %d prog: %d name: %s\n", bank, prog, preset->get_name(preset));
                              midiPatch.hbank = bank;
                              midiPatch.lbank = 0xff;  // Off
                              midiPatch.prog = prog;
                              midiPatch.name = preset->get_name (preset);
                              return &midiPatch;
                              }
                        }
                        prog = 0; // Reset if we "come around"
                  }
            }
      else { //This is a drum channel
            unsigned bank = 128;
            unsigned prog = patch->prog;
            for (prog = patch->prog + 1; prog < 128; ++prog) {
                  preset = sfont->get_preset (sfont, bank, prog);
                  if (preset) {
                        //printf("Preset info: bank: %d prog: %d name: %s\n",bank, prog, preset->get_name(preset));
                        midiPatch.hbank = 0xff;  // Off
                        midiPatch.lbank = 0xff;  // Off
                        midiPatch.prog = prog;
                        midiPatch.name = preset->get_name (preset);
                        return &midiPatch;
                        }
                  }
            }
      return 0;
      }

//---------------------------------------------------------
//   popSoundfont
//---------------------------------------------------------

bool FluidSynth::popSoundfont (int ext_id)
      {
         bool success = false;
         int int_id = getFontInternalIdByExtId(ext_id);

         //if (int_id == FS_UNSPECIFIED_FONT) {
         if (int_id == FS_UNSPECIFIED_FONT || int_id == FS_UNSPECIFIED_ID) {
               std::cerr << DEBUG_ARGS << "Internal error! Request for deletion of Soundfont that is not registered!" << std::endl;
               }
         else
         {
         //Try to unload soundfont
         int err = fluid_synth_sfunload(fluidsynth, int_id, 0);
         if (err != -1) {//Success
               //Check all channels that the font is used in
               for (int i=0; i<FS_MAX_NR_OF_CHANNELS;  i++) {
                     //Set them to unspecified and reset preset settings
                     if (channels[i].font_intid == int_id) {
                           channels[i].font_intid = FS_UNSPECIFIED_ID;
                           channels[i].font_extid = FS_UNSPECIFIED_ID;
                           channels[i].preset = FS_UNSPECIFIED_PRESET;
                           }
                     }
               //Remove it from soundfont stack
               for (std::list<FluidSoundFont>::iterator it =stack.begin(); it !=stack.end(); it++) {
                     if (it->intid == int_id) {
                           stack.erase(it);
                           break;
                           }
                     }
               //Resend fontdata & re-initialize
               sendSoundFontData();
               sendChannelData();
               rewriteChannelSettings();
               success = true;
               currentlyLoadedFonts--;
            }
         else //OK, there was trouble
               std::cerr << DEBUG_ARGS << "Error unloading soundfont!" << fluid_synth_error(fluidsynth) << std::endl;
      }
      if (FS_DEBUG)
            printf("Removed soundfont with ext it: %d\n",ext_id);
      return success;
      }


void LoadFontWorker::loadFont(void* h)
{
  emit loadFontSignal(h);
}

//---------------------------------------------------------
//   execLoadFont
//    helper function to load soundfont in the
//    background.
//---------------------------------------------------------
void LoadFontWorker::execLoadFont(void * t)
{
      FS_Helper *h = (FS_Helper*) t;
      FluidSynth* fptr = h->fptr;

      QByteArray ba = h->file_name.toLocal8Bit();
      const char* filename = ba.constData();

      if (FS_DEBUG)
         printf("execLoadFont() font name %s\n", filename);

      //Let only one loadThread have access to the fluidsynth-object at the time
      QMutexLocker ml(&fptr->_sfLoaderMutex);
      int rv = fluid_synth_sfload(fptr->fluidsynth, filename, 1);

      if (rv ==-1) {
            fptr->sendError(fluid_synth_error(fptr->fluidsynth));
            if (FS_DEBUG)
                  std::cerr << DEBUG_ARGS << "error loading soundfont: " << fluid_synth_error(fptr->fluidsynth) << std::endl;

            delete h;
            return;
      }

      //Deal with internal and external id etc.
      if (FS_DEBUG)
            printf("Soundfont %s loaded, index %d\n", filename, rv);

      FluidSoundFont font;
      font.file_name = h->file_name;

      font.intid = rv;
      if (h->id == FS_UNSPECIFIED_ID) {
            font.extid = fptr->getNextAvailableExternalId();
            if (FS_DEBUG)
                  printf("Font got extid %d\n",font.extid);
            }
      else
            font.extid = h->id;
      if (FS_DEBUG)
            printf("Font has external id: %d int id:%d\n", font.extid, font.intid);

      //Strip off the filename
      QFileInfo fi(h->file_name);
      font.name = fi.fileName();

      #ifdef HAVE_INSTPATCH
      loadNoteSampleNames(font);
      #endif

      fptr->stack.push_front(font);
      fptr->currentlyLoadedFonts++;

      if (FS_DEBUG)
            printf("Currently loaded fonts: %d Nr of soundfonts: %d\n",fptr->currentlyLoadedFonts, fptr->nrOfSoundfonts);
      //Check whether this was the last font or not. If so, run initSynth();
      if (fptr->nrOfSoundfonts <= fptr->currentlyLoadedFonts) {
            if (FS_DEBUG)
                  printf("This was the last font, rewriting channel settings...\n");
            fptr->rewriteChannelSettings();
            //Update data in GUI-window.
            fptr->sendSoundFontData();;
            fptr->sendChannelData();
            }

      delete h;
}

//---------------------------------------------------------
//   instantiate
//    construct a new synthesizer instance
//---------------------------------------------------------

class QWidget;
static  QMutex globalFluidSynthMutex;


static Mess* instantiate(int sr, QWidget*, QString* projectPathPtr, const char* name)
      {
      printf("fluidsynth sampleRate %d\n", sr);
      projPathPtr=projectPathPtr;

      FluidSynth* synth = new FluidSynth(sr, globalFluidSynthMutex);
      if (synth->init(name)) {
            delete synth;
            synth = 0;
            }
      return synth;
      }

extern "C"
      {
      static MESS descriptor = {
            "FluidSynth",
            "FluidSynth soundfont loader by Mathias Lundgren", //Mathias Lundgren (lunar_shuttle@users.sf.net)
            "0.1",      //Version string
            MESS_MAJOR_VERSION, MESS_MINOR_VERSION,
            instantiate,
            };
      // We must compile with -fvisibility=hidden to avoid namespace
      // conflicts with global variables.
      // Only visible symbol is "mess_descriptor".
      // (TODO: all plugins should be compiled this way)
  
      __attribute__ ((visibility("default")))
      const MESS* mess_descriptor() { return &descriptor; }
      }

