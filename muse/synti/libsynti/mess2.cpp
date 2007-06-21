//=========================================================
//  MusE
//  Linux Music Editor
//    $Id:$
//  (C) Copyright 2007 Werner Schweer (ws@seh.de)
//=========================================================

#include "mess2.h"

QList<SynthCtrl*> Mess2::ctrl;

//---------------------------------------------------------
//   Mess2
//---------------------------------------------------------

Mess2::Mess2(int channels)
   : Mess(channels)
      {
      initData = 0;
      }

//---------------------------------------------------------
//   Mess2
//---------------------------------------------------------

Mess2::~Mess2()
      {
      if (initData)
            delete[] initData;
      }

//---------------------------------------------------------
//   addController
//---------------------------------------------------------

void Mess2::addController(const char* name, int id, int min, int max, int init)
      {
      SynthCtrl* c = new SynthCtrl(name, id, min, max, init);
      ctrl.append(c);
      }

//---------------------------------------------------------
//   controllerIdx
//---------------------------------------------------------

int Mess2::controllerIdx(const char* name)
      {
      for (int i = 0; i < ctrl.size(); ++i) {
            if (strcmp(ctrl[i]->name, name) == 0)
                  return i;
            }
      return -1;
      }

//---------------------------------------------------------
//   controllerIdx
//---------------------------------------------------------

int Mess2::controllerIdx(int ctrlId)
      {
      for (int i = 0; i < ctrl.size(); ++i) {
            if (ctrl[i]->ctrl == ctrlId)
                  return i;
            }
      return -1;
      }

//---------------------------------------------------------
//   controllerId
//---------------------------------------------------------

int Mess2::controllerId(int idx)
      {
      if (idx < 0 || idx >= ctrl.size()) {
            printf("controllId::illegal controller index %d\n", idx);
            return -1;
            }
      return ctrl[idx]->ctrl;
      }

//---------------------------------------------------------
//   controllerName
//---------------------------------------------------------

const char* Mess2::controllerName(int idx)
      {
      if (idx < 0 || idx >= ctrl.size()) {
            printf("controllerName::illegal controller index %d\n", idx);
            return "?";
            }
      return ctrl[idx]->name;
      }

//---------------------------------------------------------
//   getInitData
//---------------------------------------------------------

void Mess2::getInitData(int* bytes, const unsigned char** data)
      {
      if (initData)
            delete[] initData;
      int n    = ctrl.size() * sizeof(int);
      initData = new unsigned char[n];
      int* p = (int*)initData;
      foreach(SynthCtrl* c, ctrl) {
            *p++ = c->val;
            }
      *data  = initData;
      *bytes = n;
      }

//---------------------------------------------------------
//   getControllerInfo
//---------------------------------------------------------

int Mess2::getControllerInfo(int idx, const char** name, int* id, int* min, int* max)
      {
      if (idx < 0 || idx >= ctrl.size())
            return 0;
      *name = ctrl[idx]->name;
      *id   = ctrl[idx]->ctrl;
      *min  = ctrl[idx]->min;
      *max  = ctrl[idx]->max;
      ++idx;
      return (idx >= ctrl.size()) ? 0 : idx;
      }


