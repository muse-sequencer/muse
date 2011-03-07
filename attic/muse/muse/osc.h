//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id: osc.h,v 1.0.0.0 2010/04/22 10:05:00 terminator356 Exp $
//
//  Copyright (C) 1999-2010 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __OSC_H__
#define __OSC_H__

#include <lo/lo.h>

#include "config.h"
 
#ifdef DSSI_SUPPORT
class DssiSynthIF;
#endif

class QProcess;
class PluginI;
class OscIF;

// Keep the OSC fifo small. There may be thousands of controls, and each control needs a fifo.
// Oops, no, if the user keeps adjusting a slider without releasing the mouse button, then all of the 
//  events are sent at once upon releasing the button, meaning there might be thousands of events at once.
#define OSC_FIFO_SIZE 512

//---------------------------------------------------------
//  OscControlValue
//  Item struct for OscGuiControlFifo. 
//---------------------------------------------------------

struct OscControlValue
{
  //int idx;
  float value;
  // maybe timestamp, too ?
};

//---------------------------------------------------------
//  OscControlFifo
//  A fifo for each of the OSC controls.
//---------------------------------------------------------

class OscControlFifo
{
      OscControlValue fifo[OSC_FIFO_SIZE];
      volatile int size;
      int wIndex;
      int rIndex;

   public:
      OscControlFifo()  { clear(); }
      bool put(const OscControlValue& event);   // returns true on fifo overflow
      OscControlValue get();
      const OscControlValue& peek(int n = 0);
      void remove();
      bool isEmpty() const { return size == 0; }
      void clear()         { size = 0, wIndex = 0, rIndex = 0; }
      int getSize() const  { return size; }
};

//---------------------------------------------------------
//  OscIF
//  Open Sound Control Interface
//---------------------------------------------------------

/*
class OscIF
{
   private:
      PluginI* _oscPluginI;
      
      #ifdef DSSI_SUPPORT
      DssiSynthIF* _oscSynthIF;
      #endif
      
      QProcess* _oscGuiQProc;
      void* _uiOscTarget;
      char* _uiOscShowPath;
      char* _uiOscControlPath;
      char* _uiOscConfigurePath;
      char* _uiOscProgramPath;
      char* _uiOscPath;
      bool _oscGuiVisible;
   
      OscControlFifo* _oscControlFifos;
      
   public:
      OscIF();
      ~OscIF();
      
      void oscSetPluginI(PluginI*);
      
      #ifdef DSSI_SUPPORT
      void oscSetSynthIF(DssiSynthIF*);
      #endif
      
      int oscUpdate(lo_arg**);    
      int oscProgram(lo_arg**);   
      int oscControl(lo_arg**);   
      int oscExiting(lo_arg**);   
      int oscMidi(lo_arg**);      
      int oscConfigure(lo_arg**); 
   
      bool oscInitGui();
      void oscShowGui(bool);
      bool oscGuiVisible() const;
      OscControlFifo* oscFifo(unsigned long) const;
};
*/ 
 
class OscIF
{
   protected:
      QProcess* _oscGuiQProc;
      void* _uiOscTarget;
      char* _uiOscPath;
      char* _uiOscSampleRatePath;
      char* _uiOscConfigurePath;
      char* _uiOscProgramPath;
      char* _uiOscControlPath;
      char* _uiOscShowPath;
      bool _oscGuiVisible;
   
      OscControlFifo* _oscControlFifos;
      
      virtual bool oscInitGui(const QString& /*typ*/, const QString& /*baseName*/, const QString& /*name*/, 
                       const QString& /*label*/, const QString& /*filePath*/, const QString& /*dirPath*/);
                       
   public:
      OscIF();
      virtual ~OscIF();
      
      OscControlFifo* oscFifo(unsigned long) const;
      
      virtual int oscUpdate(lo_arg**);    
      virtual int oscProgram(lo_arg**)   { return 0; }   
      virtual int oscControl(lo_arg**)   { return 0; }    
      virtual int oscExiting(lo_arg**);   
      virtual int oscMidi(lo_arg**)      { return 0; }      
      virtual int oscConfigure(lo_arg**) { return 0; } 
   
      virtual void oscSendProgram(unsigned long /*prog*/, unsigned long /*bank*/);    
      virtual void oscSendControl(unsigned long /*dssiPort*/, float /*val*/);    
      virtual void oscSendConfigure(const char */*key*/, const char */*val*/); 
      
      virtual bool oscInitGui() { return false; }
      virtual void oscShowGui(bool);
      virtual bool oscGuiVisible() const;
};
 
class OscEffectIF : public OscIF
{
   protected:
      PluginI* _oscPluginI;
   
   public:
      OscEffectIF() {}
      //~OscEffectIF();

      void oscSetPluginI(PluginI*);
      
      virtual int oscUpdate(lo_arg**);
      //virtual int oscProgram(lo_arg**);
      virtual int oscControl(lo_arg**);
      //virtual int oscExiting(lo_arg**);
      //virtual int oscMidi(lo_arg**);
      virtual int oscConfigure(lo_arg**);
      
      virtual bool oscInitGui();
};
 
#ifdef DSSI_SUPPORT
class OscDssiIF : public OscIF
{
   protected:
      DssiSynthIF* _oscSynthIF;
      
   public:
      OscDssiIF() {}
      //~OscDssiIF();
      
      void oscSetSynthIF(DssiSynthIF*);
      
      virtual int oscUpdate(lo_arg**);
      virtual int oscProgram(lo_arg**);
      virtual int oscControl(lo_arg**);
      //virtual int oscExiting(lo_arg**);
      virtual int oscMidi(lo_arg**);
      virtual int oscConfigure(lo_arg**);
      
      virtual bool oscInitGui();
};
#endif // DSSI_SUPPORT
 
extern void initOSC();

#endif
