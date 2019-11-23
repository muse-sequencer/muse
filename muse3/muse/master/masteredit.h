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

#include "type_defs.h"
#include "midieditor.h"
#include "noteinfo.h"
#include "cobject.h"

class QCloseEvent;
class QToolBar;

namespace MusEGui {
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
      TScale* tscale;

      MusEGui::LabelCombo* rasterLabel;
      QToolBar* tools;
      MusEGui::PosLabel* cursorPos;
      MusEGui::TempoLabel* tempo;
      
      static int _rasterInit;
      
      virtual void keyPressEvent(QKeyEvent*);
      virtual void closeEvent(QCloseEvent*);

   private slots:
      void _setRaster(int);
      void setTime(unsigned);
      void setTempo(int);

   public slots:
      void songChanged(MusECore::SongChangedStruct_t);
      void focusCanvas();

   signals:
      void isDeleting(MusEGui::TopWin*);

   public:
      MasterEdit(QWidget* parent = 0, const char* name = 0);
      ~MasterEdit();
      virtual void readStatus(MusECore::Xml&);
      virtual void writeStatus(int, MusECore::Xml&) const;
      static void readConfiguration(MusECore::Xml&);
      static void writeConfiguration(int, MusECore::Xml&);
      };

} // namespace MusEGui

#endif

