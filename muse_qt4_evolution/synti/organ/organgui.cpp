//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: organgui.cpp,v 1.21 2005/12/16 15:36:51 wschweer Exp $
//
//    This is a simple GUI implemented with QT for
//    organ software synthesizer.
//
//  (C) Copyright 2001-2007 Werner Schweer (ws@seh.de)
//=========================================================

#include "organgui.h"
#include "muse/midi.h"
#include "muse/midictrl.h"
#include "awl/knob.h"

//---------------------------------------------------------
//   OrganGui
//---------------------------------------------------------

OrganGui::OrganGui()
   : QWidget(0),
     MessGui()
      {
      setupUi(this);
      QSocketNotifier* s = new QSocketNotifier(readFd, QSocketNotifier::Read);
      connect(s, SIGNAL(activated(int)), SLOT(readMessage(int)));

      map = new QSignalMapper(this);
      QList<QWidget*> wl = findChildren<QWidget*>();
      foreach(QWidget* w, wl) {
            int idx = Mess2::controllerIdx(w->objectName().toAscii().data());
            if (idx == -1)
                  continue;
            w->setProperty("ctrlIdx", idx);
            map->setMapping(w, w);
            const char* cname = w->metaObject()->className();
            if (strcmp(cname, "QSlider") == 0) {
                  QSlider* slider = (QSlider*)w;
                  w->setProperty("ctrlType", 0);
                  connect(slider, SIGNAL(valueChanged(int)), map, SLOT(map()));
                  }
            else if (strcmp(cname, "QCheckBox") == 0) {
                  w->setProperty("ctrlType", 1);
                  connect(w, SIGNAL(toggled(bool)), map, SLOT(map()));
                  }
            else if (strcmp(cname, "QGroupBox") == 0) {
                  w->setProperty("ctrlType", 2);
                  connect(w, SIGNAL(toggled(bool)), map, SLOT(map()));
                  }
            else if (strcmp(cname, "Awl::Knob") == 0) {
                  w->setProperty("ctrlType", 3);
                  connect(w, SIGNAL(valueChanged(double,int)), map, SLOT(map()));
                  }
            else if (strcmp(cname, "QPushButton") == 0) {
                  w->setProperty("ctrlType", 4);
                  connect(w, SIGNAL(toggled(bool)), map, SLOT(map()));
                  }
            else if (strcmp(cname, "Awl::Drawbar") == 0) {
                  Awl::Drawbar* drawbar = (Awl::Drawbar*)w;
                  w->setProperty("ctrlType", 5);
                  connect(drawbar, SIGNAL(valueChanged(double,int)), map, SLOT(map()));
                  }
            else
                  printf("Gui Element <%s> not supported\n", cname);
            }
      ignoreControllerChange = false;
      connect(map, SIGNAL(mapped(QWidget*)), this, SLOT(ctrlChanged(QWidget*)));
      }

//---------------------------------------------------------
//   ctrlChanged
//---------------------------------------------------------

void OrganGui::ctrlChanged(QWidget* w)
      {
      if (ignoreControllerChange)
            return;
      int ctrlIdx  = w->property("ctrlIdx").toInt();
      int ctrlType = w->property("ctrlType").toInt();
      int value    = 0;

      switch(ctrlType) {
            case 0:     // QSlider
                  value = ((QSlider*)w)->value();
                  break;
            case 1:
                  value = ((QCheckBox*)w)->isChecked();
                  break;
            case 2:
                  value = ((QGroupBox*)w)->isChecked();
                  break;
            case 3:
                  value = lrint(((Awl::Knob*)w)->value());
                  break;
            case 4:
                  value = ((QPushButton*)w)->isChecked();
                  break;
            case 5:
                  value = lrint(((Awl::Drawbar*)w)->value());
                  break;
            default:
                  printf("OrganGui::ctrlChanged: illegal ctrlType %d\n", ctrlType);
                  break;
            }
      int id = Mess2::controllerId(ctrlIdx);
      sendController(0, id, value);      // to synth
      }

//---------------------------------------------------------
//   setParam
//---------------------------------------------------------

void OrganGui::setParam(int ctrlId, int val)
      {
      int ctrlIdx = Mess2::controllerIdx(ctrlId);
      if (ctrlIdx == -1)
            return;
      setParamIdx(ctrlIdx, val);
      }

//---------------------------------------------------------
//   setParamIdx
//    set controller value in gui
//---------------------------------------------------------

void OrganGui::setParamIdx(int ctrlIdx, int val)
      {
      const char* name = Organ::controllerName(ctrlIdx);
      if (name == 0)
            return;
      ignoreControllerChange = true;
      QList<QWidget*> wl = findChildren<QWidget*>(name);

      foreach(QWidget* w, wl) {
            int ctrlType = w->property("ctrlType").toInt();
            switch(ctrlType) {
                  case 0:
                        ((QSlider*)w)->setValue(val);
                        break;
                  case 1:
                        ((QCheckBox*)w)->setChecked(val);
                        break;
                  case 2:
                        ((QGroupBox*)w)->setChecked(val);
                        break;
                  case 3:
                        ((Awl::Knob*)w)->setValue(double(val));
                        break;
                  case 4:
                        ((QPushButton*)w)->setChecked(val);
                        break;
                  case 5:
                        ((Awl::Drawbar*)w)->setValue(double(val));
                        break;
                  default:
                        printf("OrganGui::setParamIdx: illegal ctrlType %d\n", ctrlType);
                        break;
                  }
            }
      ignoreControllerChange = false;
      }

//---------------------------------------------------------
//   processEvent
//---------------------------------------------------------

void OrganGui::processEvent(const MidiEvent& ev)
      {
      if (ev.type() == ME_CONTROLLER)
            setParam(ev.dataA(), ev.dataB());
      else
            printf("OrganGui::illegal event type received\n");
      }

//---------------------------------------------------------
//   readMessage
//---------------------------------------------------------

void OrganGui::readMessage(int)
      {
      MessGui::readMessage();
      }

