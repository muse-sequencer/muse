//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: spinbox.h,v 1.1.2.2 2009/02/02 21:38:01 terminator356 Exp $
//    (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

// SpinBox:
// Click up/down, or mousewheel, or hit enter with un-modified text (which means enter TWICE for modified text), 
// and the control will give up focus, thereby allowing you to use global shortcut keys afterwards. 
// Up/down keys still keep the focus.
//
// < Old. That was the SpinBox behaviour in MusE1 (Qt3). They are still desirable goals for MusE2 (Qt4).  
// < Flaw: Calling clearFocus() means nothing has focus, not canvases, not even the active top level window. We want canvases to have it.  
// <       That requires (here in MusE2) setting top win focus proxies and using Application::focusChanged() to redirect focus to the 
// <        active window's proxy. 
// <       Very ugly. And with MDI, even more complicated to give focus back to current sub-window. Tried, was crash-prone.
// <       Also, toolbars can be floated, so calling clearFocus() from a SpinBox on a floating toolbar means nothing has focus but the
// <        toolbar itself is the active window, which requires setting a focus proxy on the toolbar so that Application::focusChanged()
// <        can figure out who to give the focus to! 
// <       It seems we will have to use signals/slots instead of clearFocus()...
// < Flaw: Clearing focus when up/down clicked (when stepBy() is called), auto-repeat might not work because the control has lost focus.  

#ifndef __SPINBOX_H__
#define __SPINBOX_H__

#include <QSpinBox>
#include <QLineEdit>

namespace MusEGui { 

class SpinBoxLineEdit : public QLineEdit
{
  Q_OBJECT
  
  protected:
    virtual void mouseDoubleClickEvent(QMouseEvent* e);
    //virtual void mousePressEvent(QMouseEvent* e);

  signals:
    void doubleClicked();
    void ctrlDoubleClicked();
    //void ctrlClicked();

  public:
    SpinBoxLineEdit(QWidget* parent = 0) : QLineEdit(parent) {};
};

//---------------------------------------------------------
//   SpinBox
//---------------------------------------------------------

class SpinBox : public QSpinBox {
      Q_OBJECT

      bool _returnMode;
      
   protected:
      virtual void keyPressEvent(QKeyEvent*);
      virtual void wheelEvent(QWheelEvent*);

   signals:
      void doubleClicked();
      void ctrlDoubleClicked();
      //void ctrlClicked();
      void returnPressed();
      void escapePressed();

   public:
      SpinBox(QWidget* parent=0);
      SpinBox(int minValue, int maxValue, int step = 1, QWidget* parent=0);
      void setReturnMode(bool v) { _returnMode = v; }
      bool returnMode() const    { return _returnMode; }
};

} // namespace MusEGui

#endif

