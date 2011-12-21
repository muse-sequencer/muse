//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: strip.h,v 1.3.2.2 2009/11/14 03:37:48 terminator356 Exp $
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

#ifndef __STRIP_H__
#define __STRIP_H__

#include <QFrame>
#include <QIcon>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>

#include "globaldefs.h"
//#include "route.h"

class QLabel;
//class QVBoxLayout;
class QToolButton;
class QGridLayout;

namespace MusECore {
class Track;
}

namespace MusEGui {
class ComboBox;
class Meter;

static const int STRIP_WIDTH = 65;

//---------------------------------------------------------
//   Strip
//---------------------------------------------------------

class Strip : public QFrame {
      Q_OBJECT
   
   protected:
      MusECore::Track* track;
      QLabel* label;
      //QVBoxLayout* layout;
      QGridLayout* grid;
      int _curGridRow;
      MusEGui::Meter* meter[MAX_CHANNELS];
      
      QToolButton* record;
      QToolButton* solo;
      QToolButton* mute;
      QToolButton* iR; // Input routing button
      QToolButton* oR; // Output routing button
      QGridLayout* sliderGrid;
      MusEGui::ComboBox* autoType;
      void setLabelText();
      virtual void resizeEvent(QResizeEvent*);
      virtual void mousePressEvent(QMouseEvent *);

   private slots:
      void recordToggled(bool);
      void soloToggled(bool);
      void muteToggled(bool);

   protected slots:
      virtual void heartBeat();
      void setAutomationType(int t);

   public slots:
      void resetPeaks();
      virtual void songChanged(int) = 0;
      virtual void configChanged() = 0;

   public:
      Strip(QWidget* parent, MusECore::Track* t);
      ~Strip();
      void setRecordFlag(bool flag);
      MusECore::Track* getTrack() const { return track; }
      void setLabelFont();
      };

} // namespace MusEGui

#endif

