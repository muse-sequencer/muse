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

   public:
      AudioStrip(Mixer*, AudioTrack*, bool align);
      virtual void heartBeat();
      };

#endif

