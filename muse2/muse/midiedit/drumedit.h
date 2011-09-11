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

#include <values.h>
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

class MidiPart;
class DrumCanvas;
class ScrollScale;
class ScoreConfig;
class PartList;
class CtrlCanvas;
class Xml;
class DList;
class CtrlEdit;
class Part;
class SNode;

namespace MusEWidget {
class Header;
class Splitter;
class Toolbar1;
}

//---------------------------------------------------------
//   DrumEdit
//---------------------------------------------------------

class DrumEdit : public MidiEditor {
      Q_OBJECT
    
      Event selEvent;
      MidiPart* selPart;
      int selTick;
      QMenu* menuEdit, *menuFunctions, *menuFile, *menuSelect;

      MusEWidget::NoteInfo* info;
      QToolButton* srec;
      QToolButton* midiin;
      MusEWidget::EditToolBar* tools2;

      MusEWidget::Toolbar1* toolbar;
      MusEWidget::Splitter* split1;
      MusEWidget::Splitter* split2;
      QWidget* split1w1;
      DList* dlist;
      MusEWidget::Header* header;
      QToolBar* tools;
      QComboBox *stepLenWidget;

      static int _rasterInit;
      static int _dlistWidthInit, _dcanvasWidthInit;

      QAction *loadAction, *saveAction, *resetAction;
      QAction *cutAction, *copyAction, *pasteAction, *pasteDialogAction, *deleteAction;
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
      void noteinfoChanged(MusEWidget::NoteInfo::ValType type, int val);
      //CtrlEdit* addCtrl();
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

   public slots:
      void setSelection(int, Event&, Part*);
      void soloChanged(bool);       // called by Solo button
      void execDeliveredScript(int);
      void execUserScript(int);
      CtrlEdit* addCtrl();
      
      virtual void updateHScrollRange();
   signals:
      void deleted(TopWin*);

   public:
      DrumEdit(PartList*, QWidget* parent = 0, const char* name = 0, unsigned initPos = MAXINT);
      virtual ~DrumEdit();
      virtual void readStatus(Xml&);
      virtual void writeStatus(int, Xml&) const;
      static void readConfiguration(Xml& xml);
      static void writeConfiguration(int, Xml&);
      };

#endif
