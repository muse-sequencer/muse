//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: transport.h,v 1.4 2004/06/28 21:13:16 wschweer Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TRANSPORT_H__
#define __TRANSPORT_H__

#include "al/sig.h"

#include <QMouseEvent>
#include <QLabel>

namespace Awl {
      class PosEdit;
      };

using Awl::PosEdit;

class QToolButton;
class QHBoxLayout;
class QSlider;
class DoubleLabel;
class SigLabel;
class QComboBox;
class Pos;
class QLabel;

//---------------------------------------------------------
//    TempoSig
//---------------------------------------------------------

class TempoSig : public QWidget {
      DoubleLabel* l1;
      SigLabel* l2;
      QLabel* l3;
      Q_OBJECT

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

      Q_OBJECT

   private slots:
      void cposChanged(const Pos&);
      void cposChanged(int);
      void lposChanged(const Pos&);
      void rposChanged(const Pos&);
      void setRecMode(int);
      void setCycleMode(int);
      void songChanged(int);
      void syncChanged(bool);
      void jackSyncChanged(bool);
      void setRecord(bool flag);
      void stopToggled(bool);
      void playToggled(bool);
      void configChanged();

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
      QColor getHandleColor() const { return lefthandle->backgroundColor(); }
      };
#endif

