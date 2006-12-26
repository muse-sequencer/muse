//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mess.h,v 1.6 2005/05/11 14:18:48 wschweer Exp $
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MESS_H__
#define __MESS_H__

#define MESS_MAJOR_VERSION 3
#define MESS_MINOR_VERSION 1

#include "midievent.h"

class QWidget;
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
      signed char typ;    // 1 - GM  2 - GS  4 - XG  8 - LBANK  16 - HBANK
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

      virtual void process(float** data, int offset, int len) = 0;

      // The synti has to (re-)implement processEvent() or provide
      // the playNote()/setControll()/sysex() functions.
      // The even routines return true if synti is busy and the
      // event must be send again.

      virtual bool processEvent(const MidiEvent&);
      virtual bool setController(int, int, int) { return false; }
      virtual bool playNote(int, int, int) { return false; }
	virtual bool sysex(int, const unsigned char*) { return false; }

      virtual void getInitData(int*, const unsigned char**) {}
      virtual int getControllerInfo(int, const char**, int*, int*, int*) {return 0;}
      virtual const char* getPatchName(int, int, int) const { return "?"; }
      virtual const MidiPatch* getPatchInfo(int, const MidiPatch*) const { return 0; }
      virtual const char* getBankName(int) const { return 0; }

      // synthesizer -> host communication
      void sendEvent(MidiEvent);  // called from synti
      MidiEvent receiveEvent();   // called from host
      int eventsPending() const;

      // GUI interface routines
      virtual bool hasGui() const { return false; }
      virtual bool guiVisible() const { return false; }
      virtual void showGui(bool) {}
      virtual void getGeometry(int* x, int* y, int* w, int* h) const;
      virtual void setGeometry(int, int, int, int) {}
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
         // QWidget* parent allows for a threaded GUI using the Qt Library
         // can be ignored by synti
      Mess* (*instantiate)(int sr, const char* name);
      };

extern "C" {
      const MESS* mess_descriptor();
      }

#endif

