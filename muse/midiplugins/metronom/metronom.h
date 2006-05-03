//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: metronom.h,v 1.3 2005/06/12 09:22:51 wschweer Exp $
//
//    metronom  - simple midi metronom
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __METRONOM_H__
#define __METRONOM_H__

#include "../libmidiplugin/mempi.h"

//---------------------------------------------------------
//   metronom - simple midi metronom
//---------------------------------------------------------

class Metronom : public Mempi {

   protected:
      struct InitData {
            char measureNote;
            char measureVelo;
            char beatNote;
            char beatVelo;
            } data;
      MetronomGui* gui;
      friend class MetronomGui;

      unsigned int nextTick;
      unsigned int lastTo;

      virtual void process(unsigned, unsigned, MPEventList*, MPEventList*);

   public:
      Metronom(const char* name, const MempiHost*);
      ~Metronom();
      virtual bool init();

      virtual bool hasGui() const      { return true;             }
      virtual bool guiVisible() const  { return gui->isVisible(); }
      virtual void showGui(bool val)   { gui->setShown(val);      }
      virtual void getGeometry(int* x, int* y, int* w, int* h) const;
      virtual void setGeometry(int, int, int, int);

      virtual void getInitData(int*, const unsigned char**) const;
      virtual void setInitData(int, const unsigned char*);
      };

#endif

