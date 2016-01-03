//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: astrip.h,v 1.8.2.6 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2013 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __ASTRIP_H__
#define __ASTRIP_H__

#include <vector>
#include <QLabel>

#include "type_defs.h"
#include "strip.h"
// #include "knob.h" // REMOVE Tim. Trackinfo. Removed.
// #include "compact_slider.h"
//#include "route.h"

//class QDialog;
class QToolButton;
//class QAction;
//class QPopupMenu;
//class PopupMenu;
class QButton;

namespace MusECore {
class AudioTrack;
}

namespace MusEGui {
class DoubleLabel;
class EffectRack;
class Knob;
class Slider;
class CompactSlider;
class TransparentToolButton;

/* clickable label */

class ClipperLabel : public QLabel
{
Q_OBJECT
public:
    explicit ClipperLabel(QWidget* parent=0 ) : QLabel(parent) {}
    ~ClipperLabel() {}
signals:
    void clicked();
protected:
    void mousePressEvent(QMouseEvent*) { emit clicked(); }
};
  
//---------------------------------------------------------
//   AudioStrip
//---------------------------------------------------------

class AudioStrip : public Strip {
      Q_OBJECT
      
  public:      
      enum ControllerType { panType = 0, auxType, gainType };

  private:      
      int channel;
      MusEGui::Slider* slider;
      MusEGui::DoubleLabel* sl;
      EffectRack* rack;

// REMOVE Tim. Trackinfo. Changed.
//       MusEGui::Knob* pan;
//       MusEGui::DoubleLabel* panl;
//       MusEGui::Knob* gain;
//       MusEGui::DoubleLabel* gainLabel;
//       std::vector<MusEGui::Knob*> auxKnob;
//       std::vector<MusEGui::DoubleLabel*> auxLabel;
      CompactSlider* pan;
      CompactSlider* gain;
      std::vector<CompactSlider*> auxControl;

      QToolButton* stereo;
      QToolButton* pre;
      MusEGui::TransparentToolButton* off;
      MusEGui::TransparentToolButton* showArr;

      double volume;
      double panVal;

      bool _volPressed;
      bool _panPressed;

      ClipperLabel *txtCliper;
      bool _isClipped;
      double _lastClipperPeak;

      //QToolButton* iR;
      //QToolButton* oR;
      
// REMOVE Tim. Trackinfo. Changed.
//       MusEGui::Knob* addKnob(Knob::KnobType, int, MusEGui::DoubleLabel**, QLabel *name);
      CompactSlider* addController(ControllerType, int, const QString& label);
      
      void updateOffState();
      void updateVolume();
      void updatePan();
      void updateChannels();
      void updateRouteButtons();

   private slots:
      void stereoToggled(bool);
      void preToggled(bool);
      void offToggled(bool);
      void iRoutePressed();
      void oRoutePressed();
      void auxChanged(double, int);
      void gainChanged(double);
      void volumeChanged(double,int,bool);
      void volumePressed();
      void volumeReleased();
      void panChanged(double,int,bool);
      void panPressed();
      void panReleased();
      void volLabelChanged(double);
// REMOVE Tim. Trackinfo. Removed.
//       void panLabelChanged(double);
//       void auxLabelChanged(double, unsigned int);
      void volumeRightClicked(const QPoint &);
      void panRightClicked(const QPoint &);

      void resetClipper();

   protected slots:
      virtual void heartBeat();

   public slots:
      virtual void configChanged();
      virtual void songChanged(MusECore::SongChangedFlags_t);

   public:
      AudioStrip(QWidget* parent, MusECore::AudioTrack*);
      ~AudioStrip();
      ///virtual QSize minimumSizeHint () const;
      //virtual QSize sizeHint () const;
      };

} // namespace MusEGui

#endif

