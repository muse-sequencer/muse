//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: tlswidget.h,v 1.11 2005/11/29 20:49:13 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TLSWIDGET_H__
#define __TLSWIDGET_H__

class Track;
class SimpleButton;
class ArrangerTrack;
class CtrlEdit;
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
      QMenu* ctrlMenu;
      QLineEdit* nameEdit;
      ArrangerTrack* at;

      TimeCanvas* tc() const { return _tc; }
      Ctrl* ctrl() const;
      int cheight() const;

      virtual void paintEvent(QPaintEvent*);
      virtual void mousePressEvent(QMouseEvent*);
      virtual void mouseReleaseEvent(QMouseEvent*);
      virtual void mouseMoveEvent(QMouseEvent*);

      void selectController(int);

   private slots:
      void labelMinusClicked();
      void updateController();
      void showController();
      void selectionChanged();
      void configChanged();
      void controllerListChanged(int);
      void autoReadChanged();
      void selectController(QAction*);

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


