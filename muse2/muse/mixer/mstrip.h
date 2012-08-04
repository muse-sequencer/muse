//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mstrip.h,v 1.4.2.4 2009/10/25 19:26:29 lunar_shuttle Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __MSTRIP_H__
#define __MSTRIP_H__

#include "strip.h"
#include <QLabel>

class QAction;
class QDialog;
class QLabel;
class QString;

namespace MusECore {
class MidiTrack;
}

namespace MusEGui {
class DoubleLabel;
class Knob;
class Slider;
class TransparentToolButton;

//---------------------------------------------------------
//   MidiStrip
//---------------------------------------------------------

class MidiStrip : public Strip {
      Q_OBJECT

      MusEGui::Slider* slider;
      MusEGui::DoubleLabel* sl;
      MusEGui::TransparentToolButton* off;

      struct KNOB {
            MusEGui::Knob* knob;
            MusEGui::DoubleLabel* dl;
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
      void offToggled(bool);
      void iRoutePressed();
      void oRoutePressed();
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
      virtual void configChanged();

   public:
      MidiStrip(QWidget* parent, MusECore::MidiTrack*);
      };

} // namespace MusEGui

#endif



