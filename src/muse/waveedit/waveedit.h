//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: waveedit.h,v 1.3.2.8 2008/01/26 07:23:21 terminator356 Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __WAVE_EDIT_H__
#define __WAVE_EDIT_H__

#include "type_defs.h"
#include "midieditor.h"


// Forward declarations:
class QMenu;
class QWidget;
class QKeyEvent;
class QCloseEvent;
class QAction;
class QSlider;
class QToolButton;
class QPoint;
class QToolBar;
class QModelIndex;

namespace MusECore {
class PartList;
}

namespace MusEGui {
class TopWin;
class Splitter;
class PosLabel;
class EditToolBar;
class RasterLabelCombo;

//---------------------------------------------------------
//   WaveEdit
//---------------------------------------------------------

class WaveEdit : public MidiEditor {
      Q_OBJECT
    
      QSlider* ymag;
      QToolBar* tb1;
      QToolButton* solo;
      QToolButton* gridOnButton;
      MusEGui::PosLabel* pos1;
      MusEGui::PosLabel* pos2;
      QAction* selectAllAction;
      QAction* selectNoneAction;
      QAction* cutAction;
      QAction* copyAction;
      QAction* copyPartRegionAction;
      QAction* pasteAction;
//      QAction* selectPrevPartAction;
//      QAction* selectNextPartAction;
      QAction* selectRangeToSelectionAction;
      QAction* adjustWaveOffsetAction;

      QAction* evColorNormalAction;
      QAction* evColorPartsAction;
      
      MusEGui::EditToolBar* tools2;
      QMenu* menuFunctions, *menuGain, *eventColor; //*select,;
      int colorMode;
      
      MusEGui::Splitter* hsplitter;

      RasterLabelCombo* rasterLabel;

      static int _rasterInit;
      static int _trackInfoWidthInit;
      static int _canvasWidthInit;
      static int colorModeInit;

      void closeEvent(QCloseEvent*) override;
      void keyPressEvent(QKeyEvent*) override;

      void initShortcuts();
      void setEventColorMode(int);

      // Sets up a reasonable zoom minimum and/or maximum based on
      //  the current global midi division (ticks per quarter note)
      //  which has a very wide range (48 - 12288).
      // Also sets the canvas and time scale offsets accordingly.
      void setupHZoomRange();

   private slots:
      void cmd(int);
      void timeChanged(unsigned t);
      void setTime(unsigned t);
      void songChanged1(MusECore::SongChangedStruct_t);
      void soloChanged(bool flag);
      void gridOnChanged(bool flag);
      void moveVerticalSlider(int val);
      void eventColorModeChanged(int);
      void _setRaster(int raster);

   public slots:
      void configChanged();
      virtual void updateHScrollRange() override;
      void horizontalZoom(bool zoom_in, const QPoint& glob_pos);
      void horizontalZoom(int mag, const QPoint& glob_pos);
      void focusCanvas() override;

   signals:
      void isDeleting(MusEGui::TopWin*);

   public:
      WaveEdit(MusECore::PartList*, QWidget* parent = nullptr, const char* name = nullptr);
      virtual ~WaveEdit() override;
      virtual void readStatus(MusECore::Xml&) override;
      virtual void writeStatus(int, MusECore::Xml&) const override;
      static void readConfiguration(MusECore::Xml&);
      static void writeConfiguration(int, MusECore::Xml&);
      // Same as setRaster() but returns the actual value used.
      int changeRaster(int val);
      void storeSettings() override;
      };

} // namespace MusEGui

#endif

