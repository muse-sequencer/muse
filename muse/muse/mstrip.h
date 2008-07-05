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

      Awl::MidiMeterSlider* slider;
      Awl::MidiVolEntry* sl;
      QToolButton* iR;
      QToolButton* oR;

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

      void addKnob(int ctrl, int idx, const QString&, const QString&, const char*, bool, int row);

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
      void iRouteShow();
      void oRouteShow();
      void recordToggled(bool);
      void monitorToggled(bool);
      QMenu* sendMenu() const { return static_cast<QMenu*>(sender()); }

   public slots:
      virtual void songChanged(int);

   public:
      MidiStrip(Mixer*, MidiTrack*, bool align = true);
      virtual void heartBeat();
      };

//---------------------------------------------------------
//   MidiOutPortStrip
//---------------------------------------------------------

class MidiOutPortStrip : public Strip {
      Q_OBJECT

      Awl::MidiMeterSlider* slider;
      Awl::MidiVolEntry* sl;
      QToolButton* oR;
      QToolButton* iR;
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
      void iRouteShow();
      void oRouteShow();
      void syncToggled(bool) const;

   public slots:
      virtual void songChanged(int);

   public:
      MidiOutPortStrip(Mixer*, MidiOutPort*, bool align = true);
      virtual void heartBeat();
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
      void iRouteShow();
      void oRouteShow();

   public slots:
      virtual void songChanged(int);

   public:
      MidiSyntiStrip(Mixer*, MidiSynti*, bool align = true);
      virtual void heartBeat();
      };

//---------------------------------------------------------
//   MidiInPortStrip
//---------------------------------------------------------

class MidiInPortStrip : public Strip {
      Q_OBJECT

      bool activity[MIDI_CHANNELS];
      QPixmap* activityOn;
      QPixmap* activityOff;
      QLabel* channelActivity[MIDI_CHANNELS];
      QToolButton* iR;
      QToolButton* oR;
      MidiInPort* inport() const { return (MidiInPort*)track; }

   private slots:
      void muteToggled(bool);
      void soloToggled(bool);
      void iRouteShow();
      void oRouteShow();

   public slots:
      virtual void songChanged(int);

   public:
      MidiInPortStrip(Mixer*, MidiInPort*, bool align = true);
      virtual void heartBeat();
      };

#endif
