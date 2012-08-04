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

namespace MusEGui {
class HitScale;
class LabelCombo;
class Master;
class MTScale;
class PosLabel;
class ScrollScale;
class SigScale;
class TempoEdit;
class TempoLabel;
class TScale;

//---------------------------------------------------------
//   MasterEdit
//---------------------------------------------------------

class MasterEdit : public MidiEditor {
      Q_OBJECT
    
      Master* canvas;
      MusEGui::ScrollScale* hscroll;
      MusEGui::ScrollScale* vscroll;
      MusEGui::MTScale* time1;
      MusEGui::MTScale* time2;
      MusEGui::SigScale* sign;
      MusEGui::HitScale* thits;
      MusEGui::HitScale* zhits;
      TScale* tscale;

      MusEGui::TempoEdit* curTempo;
      SigEdit* curSig;
      MusEGui::LabelCombo* rasterLabel;
      QToolBar* tools;
      MusEGui::PosLabel* cursorPos;
      MusEGui::TempoLabel* tempo;
      QToolButton* enableButton;
      
      static int _rasterInit;
      
      virtual void closeEvent(QCloseEvent*);

   private slots:
      void _setRaster(int);
      void posChanged(int,unsigned,bool);
      void setTime(unsigned);
      void setTempo(int);
      void sigChange(const AL::TimeSignature&);
      void tempoChange(double);

   public slots:
      void songChanged(int);
      void focusCanvas();

   signals:
      void isDeleting(MusEGui::TopWin*);

   public:
      MasterEdit();
      ~MasterEdit();
      virtual void readStatus(MusECore::Xml&);
      virtual void writeStatus(int, MusECore::Xml&) const;
      static void readConfiguration(MusECore::Xml&);
      static void writeConfiguration(int, MusECore::Xml&);
      };

} // namespace MusEGui

#endif

