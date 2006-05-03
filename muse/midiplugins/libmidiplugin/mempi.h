//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mempi.h,v 1.10 2005/07/16 09:31:50 wschweer Exp $
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

//
//    MusE experimental midi plugin interface
//

#ifndef __MEMPI_H__
#define __MEMPI_H__

#define MEMPI_MAJOR_VERSION 1
#define MEMPI_MINOR_VERSION 1

#include <set>
#include "evdata.h"
#include "memory.h"
#include "mpevent.h"

class MempiP;

//---------------------------------------------------------
//  MempiHost
//    Host Infos
//---------------------------------------------------------

struct MempiHost {
      virtual int division() const;   // midi division
      virtual int tempo(unsigned tick) const;
      virtual unsigned tick2frame(unsigned tick) const;
      virtual unsigned frame2tick(unsigned frame) const;
      virtual void bar(int tick, int* bar, int* beat, unsigned* rest) const;
      virtual unsigned bar2tick(int bar, int beat, int tick) const;
      virtual ~MempiHost() {}
      };

//---------------------------------------------------------
//  Mempi
//    Instance class
//    MusE experimental midi plugin interface
//    Instance virtual interface class
//---------------------------------------------------------

class Mempi {
      MempiP* d;
      const char* _name;      // mempi instance name

   protected:
      const MempiHost* host;

   public:
      // modul interface
      Mempi(const char* name, const MempiHost*);
      virtual ~Mempi();
      virtual bool init()             { return false;  }
      const char* name() const        { return _name;  }

      // process interface
      virtual void process(unsigned, unsigned, MPEventList*, MPEventList*) = 0;

      // session interface
      virtual void getInitData(int*, const unsigned char**) const {}
      virtual void setInitData(int, const unsigned char*) {}

      // GUI interface routines
      virtual bool hasGui() const     { return false; }
      virtual bool guiVisible() const { return false; }
      virtual void showGui(bool)      {}
      virtual void getGeometry(int* x, int* y, int* w, int* h) const;
      virtual void setGeometry(int, int, int, int) {}
      };

//---------------------------------------------------------
//   MEMPI
//    Class descriptor
//---------------------------------------------------------

enum MempiType { MEMPI_FILTER = 0, MEMPI_GENERATOR = 1 };

struct MEMPI {
      const char* name;
      const char* description;
      const char* version;
      MempiType type;
      int majorMempiVersion, minorMempiVersion;
      Mempi* (*instantiate)(const char* name, const MempiHost*);
      };

extern "C" {
      const MEMPI* mempi_descriptor();
      }

#endif

