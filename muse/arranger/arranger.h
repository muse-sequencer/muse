//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: arranger.h,v 1.17.2.15 2009/11/14 03:37:48 terminator356 Exp $
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

#ifndef __ARRANGER_H__
#define __ARRANGER_H__

#include <vector>

#include <QScrollBar>

#include "midieditor.h"
#include "pcanvas.h"
#include "trackautomationview.h"

class QAction;
class QCheckBox;
class QMainWindow;
class QMenu;
class QToolButton;
class QWheelEvent;
class QKeyEvent;

namespace MusECore {
class Track;
class Xml;
}

namespace MusEGui {
class ArrangerView;
class AudioStrip;
class Header;
class LabelCombo;
class MTScale;
class MidiTrackInfo;
class PosLabel;
class ScrollScale;
class SpinBox;
class Splitter;
class TLLayout;
class TList;
class WidgetStack;

//---------------------------------------------------------
//   WidgetStack
//---------------------------------------------------------

class WidgetStack : public QWidget {
      Q_OBJECT
      std::vector<QWidget*> stack;
      int top;

   protected:
      virtual void wheelEvent(QWheelEvent* e);
      
   signals:
      void redirectWheelEvent(QWheelEvent*);
      
   public:
      WidgetStack(QWidget* parent, const char* name = 0);
      void raiseWidget(int idx);
      void addWidget(QWidget* w, unsigned int idx);
      QWidget* getWidget(unsigned int idx);
      QWidget* visibleWidget() const;
      int curIdx() const { return top; }
      virtual QSize minimumSizeHint() const;
      };

//---------------------------------------------------------
//   ScrollBar
//---------------------------------------------------------

class ScrollBar : public QScrollBar {
      Q_OBJECT
      
  public slots:
      void redirectedWheelEvent(QWheelEvent*);
      
  public:    
    ScrollBar(Qt::Orientation orientation, QWidget * parent = 0 ) : QScrollBar(orientation, parent) {};  
};

//---------------------------------------------------------
//   Arranger
//---------------------------------------------------------

class Arranger : public QWidget {
      Q_OBJECT

      int _quant, _raster;
      PartCanvas* canvas;
      ScrollScale* hscroll;
      QScrollBar* vscroll;
      TList* list;
      Header* header;
      MTScale* time;
      SpinBox* lenEntry;
      bool showTrackinfoFlag;
      WidgetStack* trackInfo;
      ScrollBar* infoScroll;
      MidiTrackInfo* midiTrackInfo;
      AudioStrip* waveTrackInfo;
      QWidget* noTrackInfo;
      TLLayout* tgrid;

      MusECore::Track* selected;

      LabelCombo* typeBox;
      QToolButton* ib;
      int trackInfoType;
      Splitter* split;
      int songType;
      PosLabel* cursorPos;
      SpinBox* globalTempoSpinBox;
      SpinBox* globalPitchSpinBox;
      
      unsigned cursVal;
      void genTrackInfo(QWidget* parent);
      void genMidiTrackInfo();
      void genWaveTrackInfo();
      void switchInfo(int);
      void setHeaderToolTips();
      void setHeaderWhatsThis();

   private slots:
      void _setRaster(int);
      void songlenChanged(int);
      void showTrackInfo(bool);
      void trackSelectionChanged();
      void trackInfoScroll(int);
      void songChanged(int);
      void modeChange(int);
      void setTime(unsigned);
      void globalPitchChanged(int);
      void globalTempoChanged(int);
      void setTempo50();
      void setTempo100();
      void setTempo200();
      void verticalScrollSetYpos(unsigned);
      void horizontalZoomIn();
      void horizontalZoomOut();
      void focusCanvas();
      
   signals:
      void editPart(MusECore::Track*);
      void selectionChanged();
      void dropSongFile(const QString&);
      void dropMidiFile(const QString&);
      void startEditor(MusECore::PartList*, int);
      void toolChanged(int);
      void setUsedTool(int);


   protected:
      virtual void keyPressEvent(QKeyEvent* event);

   public slots:
      void dclickPart(MusECore::Track*);
      void setTool(int);
      void updateTrackInfo(int flags);
      void configChanged();
      void controllerChanged(MusECore::Track *t);

   public:
      enum { CMD_CUT_PART, CMD_COPY_PART, CMD_COPY_PART_IN_RANGE, CMD_PASTE_PART, CMD_PASTE_CLONE_PART,
             CMD_PASTE_DIALOG, CMD_PASTE_CLONE_DIALOG, CMD_INSERT_EMPTYMEAS };

      Arranger(ArrangerView* parent, const char* name = 0);

      PartCanvas* getCanvas() { return canvas; }
      void setMode(int);
      void reset();
      
      void writeStatus(int level, MusECore::Xml&);
      void readStatus(MusECore::Xml&);

      MusECore::Track* curTrack() const { return selected; }
      void cmd(int);
      bool isSingleSelection() { return canvas->isSingleSelection(); }
      int selectionSize() { return canvas->selectionSize(); }
      void setGlobalTempo(int);
      void clear();
      void songIsClearing() { canvas->songIsClearing(); }
      
      unsigned cursorValue() { return cursVal; }
      
      ArrangerView* parentWin;
      };

} // namespace MusEGui

#endif

