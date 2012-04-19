//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: drumedit.h,v 1.9.2.7 2009/11/16 11:29:33 lunar_shuttle Exp $
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

#ifndef __DRUM_EDIT_H__
#define __DRUM_EDIT_H__

#include <QByteArray>

#include <limits.h>
#include "midieditor.h"
#include "noteinfo.h"
#include "cobject.h"
#include "tools.h"
#include "header.h"
#include "shortcuts.h"
#include "event.h"

class QCloseEvent;
class QLabel;
class QMenu;
class QKeyEvent;
class QResizeEvent;
class QToolButton;
class QWidget;
class QComboBox;


namespace MusECore {
class MidiPart;
class Part;
class PartList;
class SNode;
class Xml;
}

namespace MusEGui {

class CtrlCanvas;
class CtrlEdit;
class DList;
class DrumCanvas;
class Header;
class ScoreConfig;
class ScrollScale;
class Splitter;
class Toolbar1;


//---------------------------------------------------------
//   DrumEdit
//---------------------------------------------------------

class DrumEdit : public MidiEditor {
      Q_OBJECT
    
      QMenu* menuEdit, *menuFunctions, *menuFile, *menuSelect;

      int tickValue;
      int lenValue;
      int pitchValue;
      int veloOnValue;
      int veloOffValue;
      bool firstValueSet;
      int tickOffset;
      int lenOffset;
      int pitchOffset;
      int veloOnOffset;
      int veloOffOffset;
      bool deltaMode;
      int lastSelections;

      MusEGui::NoteInfo* info;
      QToolButton* srec;
      QToolButton* midiin;
      MusEGui::EditToolBar* tools2;

      MusEGui::Toolbar1* toolbar;
      MusEGui::Splitter* split1;
      MusEGui::Splitter* split2;
      QWidget* split1w1;
      DList* dlist;
      MusEGui::Header* header;
      QToolBar* tools;
      QComboBox *stepLenWidget;

      static int _rasterInit;
      static int _dlistWidthInit, _dcanvasWidthInit;

      QAction *loadAction, *saveAction, *resetAction;
      QAction *cutAction, *copyAction, *copyRangeAction, *pasteAction, *pasteDialogAction, *deleteAction;
      QAction *fixedAction, *veloAction, *crescAction, *quantizeAction;
      QAction *sallAction, *snoneAction, *invAction, *inAction , *outAction;
      QAction *prevAction, *nextAction;

      
      void initShortcuts();

      virtual void closeEvent(QCloseEvent*);
      QWidget* genToolbar(QWidget* parent);
      virtual void keyPressEvent(QKeyEvent*);

      void setHeaderToolTips();
      void setHeaderWhatsThis();

   private slots:
      void setRaster(int);
      void noteinfoChanged(MusEGui::NoteInfo::ValType type, int val);
      void removeCtrl(CtrlEdit* ctrl);
      void cmd(int);
      void clipboardChanged(); // enable/disable "Paste"
      void selectionChanged(); // enable/disable "Copy" & "Paste"
      void load();
      void save();
      void reset();
      void setTime(unsigned);
      void follow(int);
      void newCanvasWidth(int);
      void configChanged();
      void songChanged1(int);
      void setStep(QString);
      void deltaModeChanged(bool);

   public slots:
      void setSelection(int /*tick*/, MusECore::Event&, MusECore::Part*, bool /*update*/);
      void soloChanged(bool);       // called by Solo button
      void execDeliveredScript(int);
      void execUserScript(int);
      void focusCanvas();
      CtrlEdit* addCtrl();
      
      virtual void updateHScrollRange();
   signals:
      void isDeleting(MusEGui::TopWin*);

   public:
      DrumEdit(MusECore::PartList*, QWidget* parent = 0, const char* name = 0, unsigned initPos = INT_MAX);
      virtual ~DrumEdit();
      virtual void readStatus(MusECore::Xml&);
      virtual void writeStatus(int, MusECore::Xml&) const;
      static void readConfiguration(MusECore::Xml& xml);
      static void writeConfiguration(int, MusECore::Xml&);
      };

} // namespace MusEGui

#endif
