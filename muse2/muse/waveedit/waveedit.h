//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: waveedit.h,v 1.3.2.8 2008/01/26 07:23:21 terminator356 Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#include "midieditor.h"

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
class WaveView;

//---------------------------------------------------------
//   WaveEdit
//---------------------------------------------------------

class WaveEdit : public MidiEditor {
      Q_OBJECT
    
      WaveView* view;
      QSlider* ymag;
      QToolBar* tb1;
      QToolButton* solo;
      MusEGui::PosLabel* pos1;
      MusEGui::PosLabel* pos2;
      QAction* selectAllAction;
      QAction* selectNoneAction;
      QAction* cutAction;
      QAction* copyAction;
      QAction* pasteAction;
      

      
      virtual void closeEvent(QCloseEvent*);
      virtual void keyPressEvent(QKeyEvent*);

      QMenu* menuFunctions, *select, *menuGain;

      void initShortcuts();

   private slots:
      void cmd(int);
      void setTime(unsigned t);
      void songChanged1(int);
      void soloChanged(bool flag);
      void moveVerticalSlider(int val);

   public slots:
      void configChanged();
      virtual void updateHScrollRange();
      void horizontalZoomIn();
      void horizontalZoomOut();
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

      enum { CMD_MUTE=0, CMD_NORMALIZE, CMD_FADE_IN, CMD_FADE_OUT, CMD_REVERSE,
             CMD_GAIN_FREE, CMD_GAIN_200, CMD_GAIN_150, CMD_GAIN_75, CMD_GAIN_50, CMD_GAIN_25,
             CMD_EDIT_COPY, CMD_EDIT_CUT, CMD_EDIT_PASTE,
             CMD_EDIT_EXTERNAL,
             CMD_SELECT_ALL, CMD_SELECT_NONE };
      };

} // namespace MusEGui

#endif

