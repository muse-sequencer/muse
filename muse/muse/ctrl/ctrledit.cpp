//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "ctrledit.h"
#include "midictrl.h"
#include "widgets/simplebutton.h"
#include "widgets/utils.h"
#include "ctrl/configmidictrl.h"
#include "ctrl/ctrldialog.h"
#include "widgets/tools.h"
#include "miditrack.h"
#include "midioutport.h"

//---------------------------------------------------------
//   CtrlEdit
//---------------------------------------------------------

CtrlEdit::CtrlEdit(QWidget* parent, TimeCanvas* timeCanvas, Track* t)
   :  QObject(parent), _track(t)
      {
      y             = 0;
      _height       = 0;
      setDrawCtrlName(true);

      _tc    = timeCanvas;
      _ctrl  = &veloList;
      if (t->type() == Track::MIDI) {
            ctrlId = CTRL_VELOCITY;
            _ctrl  = &veloList;
            }
      else {
            ctrlId = AC_VOLUME;
            _ctrl  = t->getController(ctrlId);
            }

      sel = new SimpleButton(tr("Sel"), parent);
      sel->setToolTip(tr("select controller"));
      sel->setAutoRaise(false);

      minus = newMinusButton();
      minus->setParent(parent);
      minus->setToolTip(tr("remove controller view"));
      minus->setAutoRaise(false);

      connect(_track, SIGNAL(controllerChanged(int)), SLOT(controllerListChanged(int)));
      connect(sel, SIGNAL(clicked()), SLOT(showControllerList()));
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
//   setCtrl
//---------------------------------------------------------

void CtrlEdit::setCtrl(int id)
      {
      _ctrl = 0;

      if (_track->type() == Track::MIDI) {
            if (id == CTRL_VELOCITY)
                  _ctrl = &veloList;
            else if (id == CTRL_SVELOCITY)
                  _ctrl = &sveloList;
            else
                  _ctrl = _track->getController(id);
            }
      else
            _ctrl = _track->getController(id);
      
      if (!_ctrl)
            printf("CtrlEdit::setCtrl(%d): not found for track <%s>\n", id,
               _track->name().toLocal8Bit().data());
      }

//---------------------------------------------------------
//   showControllerList
//---------------------------------------------------------

void CtrlEdit::showControllerList()
      {
      Ctrl* c = ctrl();
      int id;
      if (c)
            id = c->id();
      else
            id = CTRL_NO_CTRL;
      for (;;) {
            CtrlDialog cd(_track, id);
            int rv = cd.exec();
            if (rv != 1)
                        return;
            id = cd.curId();
            if (id == CTRL_NO_CTRL)
                  return;
            if (id != CTRL_OTHER)
                  break;
            ConfigMidiCtrl* mce = new ConfigMidiCtrl((MidiTrack*)_track);
            mce->exec();
            delete mce;
            }
      changeController(id);
      }

//---------------------------------------------------------
//   changeController
//---------------------------------------------------------

void CtrlEdit::changeController(int id)
      {
      if (id == CTRL_VELOCITY) {
            ctrlId = id;
            _ctrl = &veloList;
            }
      else if (id == CTRL_SVELOCITY) {
            ctrlId = id;
            _ctrl = &sveloList;
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

