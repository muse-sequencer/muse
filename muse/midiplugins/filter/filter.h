//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: filter.h,v 1.4 2005/06/12 08:18:37 wschweer Exp $
//
//    filter  - simple midi filter
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __FILTER_H__
#define __FILTER_H__

#include "../libmidiplugin/mempi.h"

//---------------------------------------------------------
//   filter - simple midi filter
//---------------------------------------------------------

class Filter : public Mempi {
      struct initData {
            int type;
            int ctrl[4];
            } data;
      FilterGui* gui;

      bool filterEvent(const MidiEvent& event);
      virtual void process(unsigned, unsigned, MPEventList*, MPEventList*);

   public:
      Filter(const char* name, const MempiHost*);
      ~Filter();
      virtual bool init();

      int midiType() const             { return data.type;         }
      void setMidiType(int t)          { data.type = t;       }
      int midiCtrl(int i) const        { return data.ctrl[i]; }
      void setMidiCtrl(int i, int val) { data.ctrl[i] = val;  }

      virtual bool hasGui() const      { return true;             }
      virtual bool guiVisible() const  { return gui->isVisible(); }
      virtual void showGui(bool val)   { gui->setShown(val);      }
      virtual void getGeometry(int* x, int* y, int* w, int* h) const;
      virtual void setGeometry(int, int, int, int);

      virtual void getInitData(int*, const unsigned char**) const;
      virtual void setInitData(int, const unsigned char*);
      };

#endif

