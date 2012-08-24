//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: vst.h,v 1.11.2.3 2009/11/25 09:09:44 terminator356 Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
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

#ifndef __VST_H__
#define __VST_H__

#include "synth.h"

//class QMenu;

namespace MusEGui {
class PopupMenu;
}

namespace MusECore {

struct _FSTHandle;
struct _FST;

//---------------------------------------------------------
//   VstSynth
//---------------------------------------------------------

class VstSynth : public Synth {
      _FSTHandle* fstHandle;

   public:
      VstSynth(const QFileInfo& fi) : Synth(fi, fi->baseName()) {
            fstHandle = 0;
            }
      
      virtual ~VstSynth() {}
      virtual Type synthType() const { return VST_SYNTH; }
      virtual void incInstances(int val);
      virtual void* instantiate();
      virtual SynthIF* createSIF(SynthI*) const;
      };

//---------------------------------------------------------
//   VstSynthIF
//    VSTi synthesizer instance
//---------------------------------------------------------

class VstSynthIF : public SynthIF
      {
      _FST* _fst;
      bool _guiVisible;

   public:
      VstSynthIF(SynthI* s) : SynthIF(s) {
            _fst = 0;
            _guiVisible = false;
            }
            
      virtual bool initGui()       { return true; };
      virtual void guiHeartBeat()  {  }
      virtual bool guiVisible() const { return false; }
      virtual void showGui(bool) {  }
      virtual bool hasGui() const { return false; }
      virtual bool nativeGuiVisible() const;
      virtual void showNativeGui(bool v);
      virtual bool hasNativeGui() const;
      virtual void getGeometry(int*x, int*y, int*w, int*h) const { *x=0;*y=0;*w=0;*h=0; }
      virtual void setGeometry(int, int, int, int) {}
      virtual void getNativeGeometry(int*x, int*y, int*w, int*h) const { *x=0;*y=0;*w=0;*h=0; }
      virtual void setNativeGeometry(int, int, int, int) {}
      virtual void preProcessAlways() { };
      virtual iMPEvent getData(MidiPort*, MPEventList*, iMPEvent, unsigned pos, int ports, unsigned n, float** buffer) ;
      virtual bool putEvent(const MidiPlayEvent& ev);
      virtual MidiPlayEvent receiveEvent();
      virtual int eventsPending() const { return 0; }
      virtual bool init(Synth*);
      virtual int channels() const;
      virtual int totalOutChannels() const;
      virtual int totalInChannels() const;
      virtual void deactivate3();
      virtual const char* getPatchName(int, int, int, bool) const { return ""; }
      virtual const char* getPatchName(int, int, MType, bool) { return ""; }
      virtual void populatePatchPopup(PopupMenu*, int, MType, bool) {};
      virtual void write(int level, Xml& xml) const;
      virtual float getParameter(unsigned long idx) const;
      virtual void setParameter(unsigned long idx, float value);
      virtual int getControllerInfo(int, const char**, int*, int*, int*, int*) { return 0; }
      };

} // namespace MusECore

#endif

