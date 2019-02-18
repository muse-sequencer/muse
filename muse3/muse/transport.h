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

#include "type_defs.h"
#include "sig.h"
// REMOVE Tim. clip. Added.
#include "pos.h"

#include <QWidget>

class QComboBox;
class QHBoxLayout;
class QLabel;
class QSlider;
class QToolButton;
class QKeyEvent;

// REMOVE Tim. clip. Removed.
// namespace MusECore {
// class Pos;
// }

namespace MusEGui {
class PosEdit;
class DoubleLabel;
class SigLabel;
class IconButton;
class TempoEdit;
class SigEdit;

//---------------------------------------------------------
//    TempoSig
//---------------------------------------------------------

class TempoSig : public QWidget {
    Q_OBJECT
    
      IconButton* _masterButton;
      TempoEdit* l1;
      SigEdit* l2;
      QLabel* l3;
      
   private slots:
      void configChanged();
      void masterToggled(bool);

   public slots:
      void newTempo(double);
      void setTempo(int tempo);

   signals:
      void tempoChanged(int);
      void sigChanged(const MusECore::TimeSignature&);
      void masterTrackChanged(bool);
      void returnPressed();
      void escapePressed();

   public:
      TempoSig(QWidget* parent=0);
      void setTimesig(int a, int b);
      void setExternalMode(bool on);
      bool masterTrack() const;
      void setMasterTrack(bool);
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
      QComboBox* recMode;
      QComboBox* cycleMode;
      IconButton* clickButton;
      IconButton* syncButton;
      IconButton* jackTransportButton;
      QToolButton* buttons[6];      // transport buttons
      QLabel* l2;
      QLabel* l3;
      QLabel* l5;
      QLabel* l6;

      Handle *lefthandle, *righthandle;
      
 protected:
      virtual void keyPressEvent(QKeyEvent *);

 private slots:
      void cposChanged(const MusECore::Pos);
      void cposChanged(int);
      void lposChanged(const MusECore::Pos);
      void rposChanged(const MusECore::Pos);
      void setRecMode(int);
      void setCycleMode(int);
      void songChanged(MusECore::SongChangedStruct_t);
      void syncChanged(bool);
      void jackSyncChanged(bool);
      void setRecord(bool flag);
      void stopToggled(bool);
      void playToggled(bool);
      void configChanged();
      void sigChange(const MusECore::TimeSignature&);

   public slots:
      void setTempo(int tempo);
      void setTimesig(int a, int b);
// REMOVE Tim. clip. Changed.
//       void setPos(int,unsigned, bool);
      void setPos(int,const MusECore::Pos, bool);
      void setMasterFlag(bool);
      void setClickFlag(bool);
      void setSyncFlag(bool);
      void setPlay(bool f);
      void setHandleColor(QColor);

   public:
      Transport(QWidget* parent, const char* name = 0);
      QColor getHandleColor() const { return lefthandle->palette().color(QPalette::Window); }
      };

} // namespace MusEGui

#endif

