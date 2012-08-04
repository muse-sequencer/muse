//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: transport.h,v 1.4 2004/06/28 21:13:16 wschweer Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#ifndef __TRANSPORT_H__
#define __TRANSPORT_H__

#include "al/sig.h"

#include <QWidget>

namespace Awl {
      class PosEdit;
      };

using Awl::PosEdit;

class QComboBox;
class QHBoxLayout;
class QLabel;
class QSlider;
class QToolButton;

namespace MusECore {
class Pos;
}

namespace MusEGui {
class DoubleLabel;
class SigLabel;

//---------------------------------------------------------
//    TempoSig
//---------------------------------------------------------

class TempoSig : public QWidget {
    Q_OBJECT
    
      DoubleLabel* l1;
      SigLabel* l2;
      QLabel* l3;
      

   private slots:
      void configChanged();

   public slots:
      void setTempo(double);
      void setTempo(int tempo);

   signals:
      void tempoChanged(int);
      void sigChanged(const AL::TimeSignature&);

   public:
      TempoSig(QWidget* parent=0);
      void setTimesig(int a, int b);
      };

//---------------------------------------------------------
//   Handle
//---------------------------------------------------------

class Handle : public QWidget {
      QWidget* rootWin;
      int dx, dy;
      void mouseMoveEvent(QMouseEvent* ev);
      void mousePressEvent(QMouseEvent* ev);
   public:
      Handle(QWidget* r, QWidget* parent=0);
      };

class TimeLLabel;

//---------------------------------------------------------
//   Transport
//---------------------------------------------------------

class Transport : public QWidget
      {
      Q_OBJECT
    
      PosEdit* tl1;           // left mark
      PosEdit* tl2;           // right mark
      PosEdit* time1;         // tick time
      PosEdit* time2;         // SMPTE
      
      QSlider* slider;
      TempoSig* tempo;
      QHBoxLayout* tb;
      QToolButton* masterButton;
      QComboBox* recMode;
      QComboBox* cycleMode;
      QToolButton* quantizeButton;
      QToolButton* clickButton;
      QToolButton* syncButton;
      QToolButton* jackTransportButton;
      QToolButton* buttons[6];      // transport buttons
      QLabel* l2;
      QLabel* l3;
      QLabel* l5;
      QLabel* l6;

      Handle *lefthandle, *righthandle;

 private slots:
      void cposChanged(const MusECore::Pos&);
      void cposChanged(int);
      void lposChanged(const MusECore::Pos&);
      void rposChanged(const MusECore::Pos&);
      void setRecMode(int);
      void setCycleMode(int);
      void songChanged(int);
      void syncChanged(bool);
      void jackSyncChanged(bool);
      void setRecord(bool flag);
      void stopToggled(bool);
      void playToggled(bool);
      void configChanged();
      void sigChange(const AL::TimeSignature&);

   public slots:
      void setTempo(int tempo);
      void setTimesig(int a, int b);
      void setPos(int,unsigned, bool);
      void setMasterFlag(bool);
      void setClickFlag(bool);
      void setQuantizeFlag(bool);
      void setSyncFlag(bool);
      void setPlay(bool f);
      void setHandleColor(QColor);

   public:
      Transport(QWidget* parent, const char* name = 0);
      QColor getHandleColor() const { return lefthandle->palette().color(QPalette::Window); }
      };

} // namespace MusEGui

#endif

