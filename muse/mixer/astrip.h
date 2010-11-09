//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: astrip.h,v 1.8.2.6 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ASTRIP_H__
#define __ASTRIP_H__

#include <vector>

#include "strip.h"
#include "route.h"

class Slider;
class Knob;
//class QDialog;
class QToolButton;
//class QAction;
//class QPopupMenu;
class PopupMenu;
class QButton;
class TransparentToolButton;
class AudioTrack;
class DoubleLabel;

//---------------------------------------------------------
//   AudioStrip
//---------------------------------------------------------

class AudioStrip : public Strip {
      Q_OBJECT

      int channel;
      Slider* slider;
      DoubleLabel* sl;

      Knob* pan;
      DoubleLabel* panl;

      std::vector<Knob*> auxKnob;
      std::vector<DoubleLabel*> auxLabel;

      QToolButton* stereo;
      QToolButton* pre;
      TransparentToolButton* off;

      double volume;
      double panVal;

      //QToolButton* iR;
      //QToolButton* oR;
      
      Knob* addKnob(int, int, DoubleLabel**);
      
      void updateOffState();
      void updateVolume();
      void updatePan();
      void updateChannels();
      //void updateRouteMenus();

   private slots:
      void stereoToggled(bool);
      void preToggled(bool);
      void offToggled(bool);
      void iRoutePressed();
      void oRoutePressed();
      void routingPopupMenuActivated(QAction*);
      void auxChanged(double, int);
      void volumeChanged(double);
      void volumePressed();
      void volumeReleased();
      void panChanged(double);
      void panPressed();
      void panReleased();
      void volLabelChanged(double);
      void panLabelChanged(double);
      void auxLabelChanged(double, unsigned int);
      void volumeRightClicked(const QPoint &);
      void panRightClicked(const QPoint &);

   protected slots:
      virtual void heartBeat();

   public slots:
      virtual void configChanged();
      virtual void songChanged(int);

   public:
      AudioStrip(QWidget* parent, AudioTrack*);
      ~AudioStrip();
      virtual QSize minimumSizeHint () const;
      };

#endif

