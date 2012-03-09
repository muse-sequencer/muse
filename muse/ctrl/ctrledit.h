//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrledit.h,v 1.4.2.1 2008/05/21 00:28:53 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __CTRL_EDIT_H__
#define __CTRL_EDIT_H__

#include <QWidget>

#include "ctrlcanvas.h"
#include "song.h"

namespace MusECore {
class Xml;
}

#define CTRL_PANEL_FIXED_WIDTH 40

namespace MusEGui {

class MidiEditor;
class CtrlView;
class CtrlPanel;

//---------------------------------------------------------
//   CtrlEdit
//---------------------------------------------------------

class CtrlEdit : public QWidget {
      Q_OBJECT
      CtrlCanvas* canvas;
      CtrlPanel* panel;

      

   private slots:
      void destroy();

   public slots:
      void setTool(int tool);
      void setXPos(int val)           { canvas->setXPos(val); }
      void setXMag(int val)           { canvas->setXMag(val); }
      void setCanvasWidth(int w);
      void setController(int n);
      
   signals:
      void timeChanged(unsigned);
      void destroyedCtrl(CtrlEdit*);
      void enterCanvas();
      void yposChanged(int);

   public:
      CtrlEdit(QWidget*, MidiEditor* e, int xmag,
         bool expand = false, const char* name = 0);
      void readStatus(MusECore::Xml&);
      void writeStatus(int, MusECore::Xml&);
      void setController(const QString& name);
      };

} // namespace MusEGui

#endif

