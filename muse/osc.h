//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id: osc.h,v 1.0.0.0 2010/04/22 10:05:00 terminator356 Exp $
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

#ifndef __OSC_H__
#define __OSC_H__

#include "config.h"

#ifdef OSC_SUPPORT
#include <lo/lo.h>

class QProcess;
class QString;

#endif // OSC_SUPPORT

namespace MusECore { 
#ifdef OSC_SUPPORT
  
#ifdef DSSI_SUPPORT
class DssiSynthIF;
#endif

class PluginI;
class OscIF;

class OscIF
{
   protected:
      pid_t _guiPid;
      QProcess* _oscGuiQProc;
      void* _uiOscTarget;
      char* _uiOscPath;
      char* _uiOscSampleRatePath;
      char* _uiOscConfigurePath;
      char* _uiOscProgramPath;
      char* _uiOscControlPath;
      char* _uiOscShowPath;
      bool _oscGuiVisible;
   
      virtual bool oscInitGui(const QString& /*typ*/, const QString& /*baseName*/, const QString& /*name*/, 
                       const QString& /*label*/, const QString& /*filePath*/, const QString& /*guiPath*/);
                       
   public:
      OscIF();
      virtual ~OscIF();
      
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
      
      virtual QString titlePrefix() const { return QString(); }
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
      
      virtual QString titlePrefix() const; 
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
      
      virtual QString titlePrefix() const; 
};
#endif // DSSI_SUPPORT

#endif // OSC_SUPPORT
 
extern void initOSC();

} // namespace MusECore

#endif
