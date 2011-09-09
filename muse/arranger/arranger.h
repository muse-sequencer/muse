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

#include "midieditor.h"
#include "pcanvas.h"
#include "trackautomationview.h"

class QAction;
class QCheckBox;
class QMainWindow;
class QMenu;
class QScrollBar;
class QToolButton;
class QWheelEvent;
class QKeyEvent;
//class QStackedWidget;

class TList;
class Track;
class Xml;
class TLLayout;
class WidgetStack;
class AudioStrip;

namespace MusEWidget {
class Header;
class LabelCombo;
class MidiTrackInfo;
class MTScale;
class PosLabel;
class ScrollScale;
class Splitter;
class SpinBox;
}

//---------------------------------------------------------
//   WidgetStack
//---------------------------------------------------------

class WidgetStack : public QWidget {
      Q_OBJECT
      std::vector<QWidget*> stack;
      int top;

   public:
      WidgetStack(QWidget* parent, const char* name = 0);
      void raiseWidget(int idx);
      void addWidget(QWidget* w, unsigned int idx);
      QWidget* getWidget(unsigned int idx);
      QWidget* visibleWidget() const;
      int curIdx() const { return top; }
      virtual QSize minimumSizeHint() const;
      //QSize minimumSize() const;
      //int minimumHeight() const;
      };

//---------------------------------------------------------
//   Arranger
//---------------------------------------------------------

class Arranger : public QWidget {
      Q_OBJECT

      int _quant, _raster;
      PartCanvas* canvas;
      MusEWidget::ScrollScale* hscroll;
      QScrollBar* vscroll;
      TList* list;
      MusEWidget::Header* header;
      MusEWidget::MTScale* time;
      MusEWidget::SpinBox* lenEntry;
      bool showTrackinfoFlag;
      WidgetStack* trackInfo;
      //QStackedWidget* trackInfo;
      QScrollBar* infoScroll;
      //MidiTrackInfoBase* midiTrackInfo;
      MusEWidget::MidiTrackInfo* midiTrackInfo;
      AudioStrip* waveTrackInfo;
      QWidget* noTrackInfo;
      TLLayout* tgrid;

      Track* selected;

      MusEWidget::LabelCombo* typeBox;
      QToolButton* ib;
      int trackInfoType;
      MusEWidget::Splitter* split;
      ///QMenu* pop;
      int songType;
      MusEWidget::PosLabel* cursorPos;
      MusEWidget::SpinBox* globalTempoSpinBox;
      MusEWidget::SpinBox* globalPitchSpinBox;
      
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
      void headerMoved();
      void globalPitchChanged(int);
      void globalTempoChanged(int);
      void setTempo50();
      void setTempo100();
      void setTempo200();
      //void seek();
      void verticalScrollSetYpos(unsigned);
      void horizontalZoomIn();
      void horizontalZoomOut();
      
   signals:
      void redirectWheelEvent(QWheelEvent*);
      void editPart(Track*);
      void selectionChanged();
      void dropSongFile(const QString&);
      void dropMidiFile(const QString&);
      void startEditor(PartList*, int);
      void toolChanged(int);
      //void addMarker(int);
      void setUsedTool(int);


   protected:
      virtual void wheelEvent(QWheelEvent* e);
      virtual void keyPressEvent(QKeyEvent* event);

   public slots:
      void dclickPart(Track*);
      void setTool(int);
      void updateTrackInfo(int flags);
      void configChanged();
      void controllerChanged(Track *t);

   public:
      enum { CMD_CUT_PART, CMD_COPY_PART, CMD_PASTE_PART, CMD_PASTE_CLONE_PART, CMD_PASTE_PART_TO_TRACK, CMD_PASTE_CLONE_PART_TO_TRACK,
             CMD_INSERT_PART, CMD_INSERT_EMPTYMEAS };

      Arranger(QMainWindow* parent, const char* name = 0);

      PartCanvas* getCanvas() { return canvas; }
      void setMode(int);
      void reset();
      
      void writeStatus(int level, Xml&);
      void readStatus(Xml&);

      Track* curTrack() const { return selected; }
      void cmd(int);
      bool isSingleSelection() { return canvas->isSingleSelection(); }
      int selectionSize() { return canvas->selectionSize(); }
      void setGlobalTempo(int);
      void clear();
      
      unsigned cursorValue() { return cursVal; }
      };

#endif

