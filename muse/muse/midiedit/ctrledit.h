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

#ifndef __CTRL_EDIT_H__
#define __CTRL_EDIT_H__

#include "ctrl/ctrleditor.h"
#include "gui.h"

class SimpleButton;
class TimeCanvas;

//---------------------------------------------------------
//   CtrlEdit
//---------------------------------------------------------

class CtrlEdit : public QObject, public CtrlEditor {
      Q_OBJECT

    	Track* const _track;
      Track* _ctrlTrack;
      Ctrl* _ctrl;
      TimeCanvas* _tc;
      int _height;

      virtual Ctrl* ctrl() const { return _ctrl; }
      virtual TimeCanvas* tc() const { return _tc; }
      virtual Track* track() const { return _track; }
      virtual Track* ctrlTrack() const { return _ctrlTrack; }

   public:
      int ctrlId;
      int y;
      SimpleButton* minus;
      SimpleButton* sel;
      QMenu* ctrlList;
      bool _drawCtrlName;

   private slots:
      void showControllerList();
      void changeController(int);
      void controllerListChanged(int);

   public:
      CtrlEdit(QWidget*, TimeCanvas*, Track*);
      ~CtrlEdit();
      int pixel2val(int) const;
      void setHeight(int val)     { _height = val; }
      int height() const          { return _height; }
      virtual int cheight() const { return _height - splitWidth; }
      void setCtrl(Ctrl* c)       { _ctrl = c; }
      void setCtrl(int id);
      Ctrl* ctrl()                { return _ctrl; }
      void setSinglePitch(int);
      };

typedef std::vector<CtrlEdit*> CtrlEditList;
typedef CtrlEditList::iterator iCtrlEdit;
typedef CtrlEditList::const_iterator ciCtrlEdit;
#endif

