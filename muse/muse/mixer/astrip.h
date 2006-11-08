//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: astrip.h,v 1.26 2005/11/04 12:03:47 wschweer Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ASTRIP_H__
#define __ASTRIP_H__

#include "strip.h"

namespace Awl {
      class MeterSlider;
      class VolEntry;
      class VolKnob;
      class PanKnob;
      class PanEntry;
      };

class AudioTrack;
class SimpleButton;
class EffectRack;
class Mixer;

//---------------------------------------------------------
//   AudioStrip
//---------------------------------------------------------

class AudioStrip : public Strip {
      Q_OBJECT

      int channel;
      EffectRack* rack1;
      EffectRack* rack2;

      Awl::MeterSlider* slider;
      Awl::VolEntry* sl;
      double volume;

      Awl::PanKnob* pan;
      Awl::PanEntry* panl;
      double panVal;

      QToolButton* stereo;
      QToolButton* pre;

      Awl::PanKnob* addPanKnob(Awl::PanEntry**);

      QToolButton* iR;
      QToolButton* oR;
      SimpleButton* off;
      SimpleButton* record;

      void updateChannels();

   private slots:
      void stereoToggled(bool);
      void preToggled(bool);
      void offToggled(bool);
      void iRouteShow();
      void oRouteShow();
      void volumeChanged(double);
      void volumePressed();
      void volumeReleased();
      void panChanged(double);
      void setPan(double);
      void panPressed();
      void panReleased();
      void muteToggled(bool);
      void soloToggled(bool);
      void recordToggled(bool);
      void autoChanged();
      virtual void controllerChanged(int id);
      virtual void songChanged(int);
      void autoReadToggled(bool);
      void autoWriteToggled(bool);
      void updateOffState();

   protected slots:
      virtual void heartBeat();

   public:
      AudioStrip(Mixer*, AudioTrack*, bool align);
      };

#endif

