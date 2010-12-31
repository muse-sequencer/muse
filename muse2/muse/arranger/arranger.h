//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: arranger.h,v 1.17.2.15 2009/11/14 03:37:48 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

class Header;
class TList;
class ScrollScale;
class MTScale;
class Track;
class Xml;
class Splitter;
class LabelCombo;
class PosLabel;
class MidiTrackInfo;
class TLLayout;
class WidgetStack;
class AudioStrip;
class SpinBox;

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
      //QSize minimumSize() const { return minimumSizeHint(); }
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
      QScrollBar* infoScroll;
      //MidiTrackInfoBase* midiTrackInfo;
      MidiTrackInfo* midiTrackInfo;
      AudioStrip* waveTrackInfo;
      QWidget* noTrackInfo;
      TLLayout* tgrid;

      Track* selected;

      LabelCombo* typeBox;
      QToolButton* ib;
      int trackInfoType;
      Splitter* split;
      ///QMenu* pop;
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
      void headerMoved();
      void globalPitchChanged(int);
      void globalTempoChanged(int);
      void setTempo50();
      void setTempo100();
      void setTempo200();
      //void seek();
      void verticalScrollSetYpos(unsigned);
      
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

