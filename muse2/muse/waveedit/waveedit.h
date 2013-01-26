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

#include <QMenu>

#include <QWidget>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QByteArray>

#include "type_defs.h"
#include "midieditor.h"
#include "tools.h"

class QAction;
class QResizeEvent;
class QSlider;
class QToolButton;

namespace MusECore {
class PartList;
}

namespace MusEGui {

class PosLabel;
class ScrollScale;
class SNode;
class WaveCanvas;

//---------------------------------------------------------
//   WaveEdit
//---------------------------------------------------------

class WaveEdit : public MidiEditor {
      Q_OBJECT
    
      QSlider* ymag;
      QToolBar* tb1;
      QToolButton* solo;
      MusEGui::PosLabel* pos1;
      MusEGui::PosLabel* pos2;
      QAction* selectAllAction;
      QAction* selectNoneAction;
      QAction* cutAction;
      QAction* copyAction;
      QAction* copyPartRegionAction;
      QAction* pasteAction;
      QAction* selectPrevPartAction;
      QAction* selectNextPartAction;
      QAction* adjustWaveOffsetAction;

      QAction* evColorNormalAction;
      QAction* evColorPartsAction;
      
      MusEGui::EditToolBar* tools2;
      QMenu* menuFunctions, *select, *menuGain, *eventColor;
      int colorMode;
      static int _rasterInit;
      static int colorModeInit;

      virtual void closeEvent(QCloseEvent*);
      virtual void keyPressEvent(QKeyEvent*);

      void initShortcuts();
      void setEventColorMode(int);

   private slots:
      void cmd(int);
      void timeChanged(unsigned t);
      void setTime(unsigned t);
      void songChanged1(MusECore::SongChangedFlags_t);
      void soloChanged(bool flag);
      void moveVerticalSlider(int val);
      void eventColorModeChanged(int);

   public slots:
      void configChanged();
      virtual void updateHScrollRange();
      void horizontalZoom(bool zoom_in, int pos_offset);
      void focusCanvas();

   signals:
      void isDeleting(MusEGui::TopWin*);

   public:
      WaveEdit(MusECore::PartList*);
      ~WaveEdit();
      virtual void readStatus(MusECore::Xml&);
      virtual void writeStatus(int, MusECore::Xml&) const;
      static void readConfiguration(MusECore::Xml&);
      static void writeConfiguration(int, MusECore::Xml&);
      };

} // namespace MusEGui

#endif

