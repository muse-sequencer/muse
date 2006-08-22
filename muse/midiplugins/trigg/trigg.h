//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: filter.h,v 1.4 2005/06/12 08:18:37 wschweer Exp $
//
//    filter  - simple midi filter
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TRIGG_H__
#define __TRIGG_H__

#include "../libmidiplugin/mempi.h"

//---------------------------------------------------------
//   filter - simple midi filter
//---------------------------------------------------------
class Trigg : public Mempi {
      struct initData {
            int note;
            int velocity;
            } data;
      friend class TriggGui;

      TriggGui* gui;

      virtual void process(unsigned, unsigned, MPEventList*, MPEventList*);

   public:
      Trigg(const char* name, const MempiHost*);
      ~Trigg();
      virtual bool init();

      void setNote(int t)          { data.note = t;       }
      void setVelocity(int t) { data.velocity = t;  }

      virtual bool hasGui() const      { return true;             }
      virtual bool guiVisible() const  { return gui->isVisible(); }
      virtual void showGui(bool val)   { gui->setShown(val);      }
      virtual void getGeometry(int* x, int* y, int* w, int* h) const;
      virtual void setGeometry(int, int, int, int);

      virtual void getInitData(int*, const unsigned char**) const;
      virtual void setInitData(int, const unsigned char*);
      };

#endif

