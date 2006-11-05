//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mstrip.h,v 1.27 2005/10/03 21:38:14 wschweer Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MSTRIP_H__
#define __MSTRIP_H__

#include "strip.h"

namespace Awl {
      class MidiMeterSlider;
      class MidiMeter;
      class FloatEntry;
      class MidiVolEntry;
      class Knob;
      };

class MidiTrack;
class MidiOutPort;
class MidiInPort;
class MidiChannel;
class MidiSynti;

//---------------------------------------------------------
//   MidiStrip
//---------------------------------------------------------

class MidiStrip : public Strip {
      Q_OBJECT

      Awl::MidiMeter* meter;
      QToolButton* iR;
      QToolButton* oR;

   private slots:
      void muteToggled(bool);
      void soloToggled(bool);
      void recordToggled(bool);
      void iRoutePressed();
      void oRoutePressed();
      void monitorToggled(bool);

   protected slots:
      virtual void heartBeat();

   public slots:
      virtual void songChanged(int);

   public:
      MidiStrip(Mixer*, MidiTrack*, bool align);
      };

//---------------------------------------------------------
//   MidiChannelStrip
//---------------------------------------------------------

class MidiChannelStrip : public Strip {
      Q_OBJECT

      Awl::MidiMeterSlider* slider;
      Awl::MidiVolEntry* sl;
      QToolButton* iR;

      struct KNOB {
            Awl::Knob* knob;
            Awl::FloatEntry* dl;
            QLabel* lb;
            } controller[4];    // pan variation reverb chorus

      bool volumeTouched;
      bool panTouched;
      bool reverbSendTouched;
      bool variSendTouched;
      bool chorusSendTouched;

      void addKnob(int ctrl, int idx, const QString&, const QString&, const char*, bool);

   private slots:
      virtual void controllerChanged(int id);
      void ctrlChanged(double val, int num);
      void muteToggled(bool);
      void soloToggled(bool);
      void autoChanged();
      void sliderPressed(int);
      void sliderReleased(int);
      void autoReadToggled(bool);
      void autoWriteToggled(bool);
      void iRoutePressed();

   protected slots:
      virtual void heartBeat();

   public slots:
      virtual void songChanged(int);

   public:
      MidiChannelStrip(Mixer*, MidiChannel*, bool align = true);
      };

//---------------------------------------------------------
//   MidiOutPortStrip
//---------------------------------------------------------

class MidiOutPortStrip : public Strip {
      Q_OBJECT

      Awl::MidiMeterSlider* slider;
      Awl::MidiVolEntry* sl;
      QToolButton* oR;
      SimpleButton* sync;

      bool volumeTouched;

   private slots:
      virtual void controllerChanged(int id);
      void ctrlChanged(double val, int num);
      void muteToggled(bool);
      void soloToggled(bool);
      void autoChanged();
      void sliderPressed(int);
      void sliderReleased(int);
      void autoReadToggled(bool);
      void autoWriteToggled(bool);
      void oRoutePressed();
      void syncToggled(bool) const;

   protected slots:
      virtual void heartBeat();

   public slots:
      virtual void songChanged(int);

   public:
      MidiOutPortStrip(Mixer*, MidiOutPort*, bool align = true);
      };

//---------------------------------------------------------
//   MidiSyntiStrip
//---------------------------------------------------------

class MidiSyntiStrip : public Strip {
      Q_OBJECT

      Awl::MidiMeterSlider* slider;
      Awl::MidiVolEntry* sl;
      QToolButton* iR;
      QToolButton* oR;

      bool volumeTouched;

   private slots:
      virtual void controllerChanged(int id);
      void ctrlChanged(double val, int num);
      void muteToggled(bool);
      void soloToggled(bool);
      void autoChanged();
      void sliderPressed(int);
      void sliderReleased(int);
      void autoReadToggled(bool);
      void autoWriteToggled(bool);
      void oRoutePressed();
      void iRoutePressed();

   protected slots:
      virtual void heartBeat();

   public slots:
      virtual void songChanged(int);

   public:
      MidiSyntiStrip(Mixer*, MidiSynti*, bool align = true);
      };

//---------------------------------------------------------
//   MidiInPortStrip
//---------------------------------------------------------

class MidiInPortStrip : public Strip {
      Q_OBJECT

      Awl::MidiMeter* meter;
      QToolButton* iR;
      QToolButton* oR;

   private slots:
      void muteToggled(bool);
      void soloToggled(bool);
      void iRoutePressed();
      void oRoutePressed();

   protected slots:
      virtual void heartBeat();

   public slots:
      virtual void songChanged(int);

   public:
      MidiInPortStrip(Mixer*, MidiInPort*, bool align = true);
      };

#endif
