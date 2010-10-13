//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mstrip.h,v 1.4.2.4 2009/10/25 19:26:29 lunar_shuttle Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MSTRIP_H__
#define __MSTRIP_H__

#include "strip.h"
//Added by qt3to4:
#include <QLabel>

class Slider;
class DoubleLabel;
class QDialog;
class Knob;
class QString;
class MidiTrack;
class QLabel;

//---------------------------------------------------------
//   MidiStrip
//---------------------------------------------------------

class MidiStrip : public Strip {
      Q_OBJECT

      Slider* slider;
      DoubleLabel* sl;
      //QToolButton* route;
      //QToolButton* iR;
      //QToolButton* oR;

      struct KNOB {
            Knob* knob;
            DoubleLabel* dl;
            QLabel* lb;
            } controller[4];    // pan variation reverb chorus

      int volume;
      int variSend;
      int reverbSend;
      int chorusSend;
      int pan;
      bool inHeartBeat;

      void addKnob(int idx, const QString&, const QString&, const char*, bool);
      void ctrlChanged(int num, int val);
      void updateControls();
      void updateOffState();
   
   private slots:
      //void routeClicked();
      void iRoutePressed();
      void oRoutePressed();
      void routingPopupMenuActivated(int /*id*/);
      void setVolume(double);
      void setPan(double);
      void setChorusSend(double);
      void setVariSend(double);
      void setReverbSend(double);
      void labelDoubleClicked(int);
      void volLabelChanged(double);
      void controlRightClicked(const QPoint&, int);

   protected slots:
      virtual void heartBeat();

   public slots:
      virtual void songChanged(int);

   public:
      MidiStrip(QWidget* parent, MidiTrack*);
      };


#endif



