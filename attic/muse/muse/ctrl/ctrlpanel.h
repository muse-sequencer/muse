//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrlpanel.h,v 1.2.2.5 2009/06/10 00:34:59 terminator356 Exp $
//  (C) Copyright 1999-2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CTRL_PANEL_H__
#define __CTRL_PANEL_H__

#include <qwidget.h>

class MidiController;
class QLabel;
class QPopupMenu;
class QPushButton;
class MidiEditor;
class Knob;
class DoubleLabel;
class MidiPort;
class MidiTrack;

//---------------------------------------------------------
//   CtrlPanel
//---------------------------------------------------------

class CtrlPanel: public QWidget {
      QPopupMenu* pop;
      QPushButton* selCtrl;
      MidiEditor* editor;
      
      MidiTrack* _track;
      MidiController* _ctrl;
      int _dnum;
      bool inHeartBeat;
      Knob* _knob;
      DoubleLabel* _dl;
      int _val;
      
      Q_OBJECT

   signals:
      void destroyPanel();
      void controllerChanged(int);

   private slots:
      void ctrlChanged(double val);
      void labelDoubleClicked();
      void ctrlRightClicked(const QPoint& p, int id);
      //void ctrlReleased(int id);
      
   protected slots:
      virtual void heartBeat();
      
   public slots:
      void setHeight(int);
      void ctrlPopup();

   public:
      CtrlPanel(QWidget*, MidiEditor*, const char* name = 0);
      void setHWController(MidiTrack* t, MidiController* ctrl);
      };
#endif
