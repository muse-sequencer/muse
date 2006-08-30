//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: filter.h,v 1.4 2005/06/12 08:18:37 wschweer Exp $
//
//    dump- simple midi event dump for testing purposes
//
//  (C) Copyright 2006 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __DUMP_H__
#define __DUMP_H__

#include "../libmidiplugin/mempi.h"

//---------------------------------------------------------
//   dump - simple midi event dump
//---------------------------------------------------------

class Dump : public Mempi {
      virtual void process(unsigned, unsigned, MPEventList*, MPEventList*);

   public:
      Dump(const char* name, const MempiHost* h) :Mempi(name, h) {}
      };

#endif

