//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: masteredit.h,v 1.3.2.2 2009/04/01 01:37:11 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MASTER_EDIT_H__
#define __MASTER_EDIT_H__

#include <QByteArray>
#include <QResizeEvent>

#include "midieditor.h"
#include "noteinfo.h"
#include "cobject.h"

namespace Awl {
      class SigEdit;
      };
using Awl::SigEdit;

class QCloseEvent;
class QToolBar;
class QToolButton;

class Master;
class ScrollScale;
class MTScale;
class SigScale;
class HitScale;
class TScale;
class TempoEdit;
class LabelCombo;
class PosLabel;
class TempoLabel;

//---------------------------------------------------------
//   MasterEdit
//---------------------------------------------------------

class MasterEdit : public MidiEditor {
      Q_OBJECT
    
      Master* canvas;
      ScrollScale* hscroll;
      ScrollScale* vscroll;
      MTScale* time1;
      MTScale* time2;
      SigScale* sign;
      HitScale* thits;
      HitScale* zhits;
      TScale* tscale;

      TempoEdit* curTempo;
      SigEdit* curSig;
      LabelCombo* rasterLabel;
      QToolBar* tools;
      PosLabel* cursorPos;
      TempoLabel* tempo;
      QToolButton* enableButton;
      
      static int _rasterInit;
      static int _widthInit, _heightInit;
      static QByteArray _toolbarInit;

      
      virtual void closeEvent(QCloseEvent*);
      virtual void resizeEvent(QResizeEvent*);
      virtual void focusOutEvent(QFocusEvent*);
      void storeInitialState();

   private slots:
      void _setRaster(int);
      void posChanged(int,unsigned,bool);
      void setTime(unsigned);
      void setTempo(int);

   public slots:
      void songChanged(int);
//      void tempoChanged(double);

   signals:
      void deleted(unsigned long);

   public:
      MasterEdit();
      ~MasterEdit();
      virtual void readStatus(Xml&);
      virtual void writeStatus(int, Xml&) const;
      static void readConfiguration(Xml&);
      static void writeConfiguration(int, Xml&);
      };

#endif

