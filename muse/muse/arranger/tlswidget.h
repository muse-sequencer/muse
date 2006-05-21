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

#ifndef __TLSWIDGET_H__
#define __TLSWIDGET_H__

class Track;
class ArrangerTrack;
class CtrlDialog;
class TimeCanvas;

#include "ctrl/ctrleditor.h"

//---------------------------------------------------------
//   TLSWidget
//---------------------------------------------------------

class TLSWidget : public QWidget, public CtrlEditor {
      Q_OBJECT

      enum { S_NORMAL, S_DRAGTOP, S_DRAGBOTTOM, S_DRAG };
      int state;

      int trackIdx;
      int starty;
      Track* _track;          // editor canvas is associated to this track
      Track* _ctrlTrack;      // track were ctrl belongs to
      TimeCanvas* _tc;
      QToolButton* ctrlList;
//      CtrlDialog* ctrlMenu;
      QLineEdit* nameEdit;
      ArrangerTrack* at;

      TimeCanvas* tc() const { return _tc; }
      Ctrl* ctrl() const;
      int cheight() const;

      virtual void paintEvent(QPaintEvent*);
      virtual void mousePressEvent(QMouseEvent*);
      virtual void mouseReleaseEvent(QMouseEvent*);
      virtual void mouseMoveEvent(QMouseEvent*);

   private slots:
      void labelMinusClicked();
      void updateController();
      void showControllerList();
      void selectionChanged();
      void configChanged();
      void controllerListChanged(int);
      void autoReadChanged();

   signals:
      void minusClicked(TLSWidget*);
      void controllerChanged(int);
      void startDrag(int idx);
      void drag(int idx, int);

   public:
      TLSWidget(Track*, ArrangerTrack*, TimeCanvas* tc);
      Track* track() const     { return _track; }
      Track* ctrlTrack() const { return _ctrlTrack; }
      void setCtrl(int ctrl);
      void setIdx(int n) { trackIdx = n; }
      };

#endif


