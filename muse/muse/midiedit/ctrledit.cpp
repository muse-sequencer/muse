//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrledit.cpp,v 1.11 2006/02/08 17:33:41 wschweer Exp $
//  (C) Copyright 1999-2005 Werner Schweer (ws@seh.de)
//=========================================================

#include "ctrledit.h"
#include "midictrl.h"
#include "widgets/simplebutton.h"
#include "widgets/utils.h"
#include "ctrl/configmidictrl.h"
#include "widgets/tools.h"
#include "miditrack.h"

static Ctrl veloList(CTRL_VELOCITY, "velocity", Ctrl::DISCRETE |Ctrl::INT, 0.0, 127.0);    // dummy
static Ctrl sveloList(CTRL_SVELOCITY, "single velocity", Ctrl::DISCRETE |Ctrl::INT, 0.0, 127.0);    // dummy

//---------------------------------------------------------
//   CtrlEdit
//---------------------------------------------------------

CtrlEdit::CtrlEdit(QWidget* parent, TimeCanvas* timeCanvas, Track* t)
   :  QObject(parent), _track(t)
      {
      _ctrlTrack    = t;
      y             = 0;
      _height       = 0;
      _drawCtrlName = true;

      _tc    = timeCanvas;
      _ctrl   = &veloList;
      ctrlId = CTRL_VELOCITY;

      connect(_track, SIGNAL(controllerChanged(int)), SLOT(controllerListChanged(int)));

      sel = new SimpleButton(QString("Sel"), parent);
      sel->setAutoRaise(false);
      minus = newMinusButton(parent);
      minus->setAutoRaise(false);

      ctrlList = new QMenu;
      sel->setMenu(ctrlList);
      sel->setPopupMode(QToolButton::InstantPopup);
      connect(ctrlList, SIGNAL(aboutToShow()), SLOT(populateController()));
      connect(ctrlList, SIGNAL(triggered(QAction*)), SLOT(changeController(QAction*)));
      }

//---------------------------------------------------------
//   CtrlEdit
//---------------------------------------------------------

CtrlEdit::~CtrlEdit()
      {
      delete sel;
      delete minus;
      }

//---------------------------------------------------------
//   populateController
//---------------------------------------------------------

void CtrlEdit::populateController()
      {
      populateControllerMenu(ctrlList);
      }

//---------------------------------------------------------
//   changeController
//---------------------------------------------------------

void CtrlEdit::changeController(QAction* a)
      {
      if (a == 0)
            return;
      int id = a->data().toInt();

      if (id == CTRL_VELOCITY) {
            ctrlId = id;
            _ctrl = &veloList;
            _ctrlTrack = _track;
            }
      else if (id == CTRL_SVELOCITY) {
            ctrlId = id;
            _ctrl = &sveloList;
            _ctrlTrack = _track;
            }
      else if (id == CTRL_OTHER) {   // "other"
            if (track()->type() == Track::MIDI) {
                  ConfigMidiCtrl* mce = new ConfigMidiCtrl((MidiTrack*)track());
                  mce->exec();
                  sel->showMenu();
                  }
            else
                  printf("CtrlEdit::changeController: not impl.\n");
            }
      else {
            ctrlId = id;
            _ctrl   = track()->getController(ctrlId);
            if (_ctrl == 0 && _track->type() == Track::MIDI) {
                  MidiChannel* mc = ((MidiTrack*)_track)->channel();
                  _ctrl = mc->getController(ctrlId);
                  _ctrlTrack = mc;
                  }
            else
                  _ctrlTrack = _track;
            }
      _tc->updateCanvasB();
      }

//---------------------------------------------------------
//   controllerListChanged
//---------------------------------------------------------

void CtrlEdit::controllerListChanged(int id)
      {
      if (id != ctrlId)
            return;
      tc()->widget()->update(tc()->rCanvasB);
      }

//---------------------------------------------------------
//   pixel2val
//---------------------------------------------------------

int CtrlEdit::pixel2val(int y) const
      {
      if (ctrl() == 0)
            return 0;
      CVal val = ctrl()->pixel2val(y, _height - splitWidth);
      return val.i;
      }

//---------------------------------------------------------
//   setSinglePitch
//---------------------------------------------------------

void CtrlEdit::setSinglePitch(int val)
      {
	singlePitch = val;
      }

