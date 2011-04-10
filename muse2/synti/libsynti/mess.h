//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mess.h,v 1.3.2.3 2009/11/19 04:20:33 terminator356 Exp $
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MESS_H__
#define __MESS_H__

#define MESS_MAJOR_VERSION 1
#define MESS_MINOR_VERSION 1

#include "mpevent.h"

class QWidget;
class QString;
class MessP;

//---------------------------------------------------------
//   MidiPatch
//---------------------------------------------------------

#define MP_TYPE_GM 1
#define MP_TYPE_GS 2
#define MP_TYPE_XG 4
#define MP_TYPE_LBANK 8
#define MP_TYPE_HBANK 16

struct MidiPatch {
      signed char typ;                     // 1 - GM  2 - GS  4 - XG
      signed char hbank, lbank, prog;
      const char* name;
      };

//---------------------------------------------------------
//  Mess
//    MusE experimental software synth
//    Instance virtual interface class
//---------------------------------------------------------

class Mess {
      MessP* d;

      int _sampleRate;
      int _channels;                // 1 - mono,  2 - stereo

   public:
      Mess(int channels);
      virtual ~Mess();

      int channels() const       { return _channels;   }
      int sampleRate() const     { return _sampleRate; }
      void setSampleRate(int r)  { _sampleRate = r;    }

      virtual void processMessages() { };
      virtual void process(float** data, int offset, int len) = 0;

      // the synti has to (re-)implement processEvent() or provide
      // some of the next three functions:

      virtual bool processEvent(const MidiPlayEvent&);
      virtual bool setController(int, int, int) { return false; }
      virtual bool playNote(int, int, int) { return false; }
	virtual bool sysex(int, const unsigned char*) { return false; }

      virtual void getInitData(int*, const unsigned char**) const {}
      virtual int getControllerInfo(int, const char**, int*, int*, int*, int*) const {return 0;}
      virtual const char* getPatchName(int, int, int, bool) const { return "?"; }
      virtual const MidiPatch* getPatchInfo(int, const MidiPatch*) const { return 0; }

      // synthesizer -> host communication
      void sendEvent(MidiPlayEvent);  // called from synti
      MidiPlayEvent receiveEvent();   // called from host
      int eventsPending() const;

      // GUI interface routines
      virtual bool hasGui() const { return false; }
      virtual bool guiVisible() const { return false; }
      virtual void showGui(bool) {}
      virtual bool hasNativeGui() const { return false; }
      virtual bool nativeGuiVisible() const { return false; }
      virtual void showNativeGui(bool) {}
      virtual void getGeometry(int* x, int* y, int* w, int* h) const;
      virtual void setGeometry(int, int, int, int) {}
      virtual void getNativeGeometry(int* x, int* y, int* w, int* h) const;
      virtual void setNativeGeometry(int, int, int, int) {}
      };

//---------------------------------------------------------
//   MESS
//    Class descriptor
//---------------------------------------------------------

struct MESS {
      const char* name;
      const char* description;
      const char* version;
      int majorMessVersion, minorMessVersion;
      Mess* (*instantiate)(int sr, QWidget* parent, QString* projectPathPtr, const char* name);
      };

extern "C" {
      const MESS* mess_descriptor();
      }

#endif

