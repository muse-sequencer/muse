//=========================================================
//  MusE
//  Linux Music Editor
//    $Id:$
//  (C) Copyright 2007 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MESS2_H__
#define __MESS2_H__

#include "mess.h"

//---------------------------------------------------------
//   SynthCtlr
//---------------------------------------------------------

struct SynthCtrl {
      const char* name;
      int ctrl;
      int min;
      int max;
      int init;
      int val;
      SynthCtrl(const char* n, int i, int a, int b, int c)
         : name(n), ctrl(i), min(a), max(b), init(c)
            {}
      };

//---------------------------------------------------------
//  Mess2
//    MusE experimental software synth
//    extended interface
//---------------------------------------------------------

class Mess2 : public Mess {
      unsigned char* initData;

      void getInitData(int*, const unsigned char**);
      int getControllerInfo(int, const char**, int*, int*, int*);

   protected:
      static QList<SynthCtrl*> ctrl;

      static void addController(const char* name,
         int ctrl, int min = 0, int max = 16384, int init = 0);
      static int controllerIdx(const char* name);
      static int controllerId(int idx);
      static int controllerIdx(int id);
      static const char* controllerName(int idx);

   public:
      Mess2(int channels);
      virtual ~Mess2();
      };

#endif

