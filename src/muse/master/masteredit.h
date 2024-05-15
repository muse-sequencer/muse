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

#include "type_defs.h"
#include "midieditor.h"
#include "noteinfo.h"
#include "cobject.h"

#include <QMetaObject>

// Forward declarations:
class QCloseEvent;
class QToolBar;
class QToolButton;

namespace MusEGui {
class Master;
class MTScale;
class PosLabel;
class SigScale;
class TempoEdit;
class TempoLabel;
class TScale;
class Xml;
class RasterLabelCombo;
class EditToolBar;

//---------------------------------------------------------
//   MasterEdit
//---------------------------------------------------------

class MasterEdit : public MidiEditor {
      Q_OBJECT

      Master* canvas;
      MusEGui::MTScale* time1;
      MusEGui::MTScale* time2;
      MusEGui::SigScale* sign;
      TScale* tscale;

      RasterLabelCombo* rasterLabel;
      QToolBar* tools;
      MusEGui::PosLabel* cursorPos;
      MusEGui::TempoLabel* tempo;
      MusEGui::EditToolBar* tools2;
      QToolButton* gridOnButton;
      int editTools;
      
      QMetaObject::Connection _configChangedMetaConn;

      static int _rasterInit;
      
      virtual void keyPressEvent(QKeyEvent*);
      virtual void closeEvent(QCloseEvent*);

      // Sets up a reasonable zoom minimum and/or maximum based on
      //  the current global midi division (ticks per quarter note)
      //  which has a very wide range (48 - 12288).
      // Also sets the canvas and time scale offsets accordingly.
      void setupHZoomRange();

   private slots:
      void _setRaster(int raster);
      void setTime(unsigned);
      void setTempo(int);
      void configChanged();
      void gridOnChanged(bool v);

   public slots:
      void songChanged(MusECore::SongChangedStruct_t);
      void focusCanvas();

   signals:
      void isDeleting(MusEGui::TopWin*);

   public:
      MasterEdit(QWidget* parent = 0, const char* name = 0);
      virtual ~MasterEdit();
      virtual void readStatus(MusECore::Xml&);
      virtual void writeStatus(int, MusECore::Xml&) const;
      static void readConfiguration(MusECore::Xml&);
      static void writeConfiguration(int, MusECore::Xml&);
      // Same as setRaster() but returns the actual value used.
      int changeRaster(int val);
      void setEditTool(int tool);
      int getEditTools() { return editTools; };
      };

} // namespace MusEGui

#endif

