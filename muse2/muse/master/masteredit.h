//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: masteredit.h,v 1.3.2.2 2009/04/01 01:37:11 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

