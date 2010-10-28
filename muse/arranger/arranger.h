//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: arranger.h,v 1.17.2.15 2009/11/14 03:37:48 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ARRANGER_H__
#define __ARRANGER_H__

#include <vector>

#include <QWidget>
#include <q3header.h>
#include <q3whatsthis.h>
//Added by qt3to4:
#include <QWheelEvent>
#include <Q3PopupMenu>

#include "midieditor.h"
#include "pcanvas.h"
#include "mtrackinfobase.h"
#include "trackautomationview.h"

class QMainWindow;
class QToolButton;
class Header;
class TList;
class ScrollScale;
class QScrollBar;
class MTScale;
class Track;
class Xml;
class Splitter;
class LabelCombo;
class PosLabel;
class QCheckBox;
class MidiTrackInfoBase;
class WaveTrackInfoBase;
class TLLayout;
class WidgetStack;
class AudioStrip;
class SpinBox;
//---------------------------------------------------------
//   TWhatsThis
//---------------------------------------------------------

class TWhatsThis : public Q3WhatsThis {
      Header* header;
   protected:
      QString text(const QPoint&);
   public:
      TWhatsThis(QWidget* parent, Header* h) : Q3WhatsThis(parent) {
            header = h;
            }
      };

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
      QSize minimumSizeHint() const;
      };

//---------------------------------------------------------
//   MidiTrackInfo
//---------------------------------------------------------

class MidiTrackInfo : public MidiTrackInfoBase {
   public:
      bool _midiDetect;
      MidiTrackInfo(QWidget* parent) : MidiTrackInfoBase(parent) { _midiDetect = false; }
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
      Q3PopupMenu* pop;
      int songType;
      PosLabel* cursorPos;
      SpinBox* globalTempoSpinBox;
      SpinBox* globalPitchSpinBox;
      int program, pan, volume;
      
      unsigned cursVal;
      void genTrackInfo(QWidget* parent);
      void genMidiTrackInfo();
      void genWaveTrackInfo();
      void updateMidiTrackInfo(int flags);
      void switchInfo(int);

   private slots:
      void _setRaster(int);
      void songlenChanged(int);
      void showTrackInfo(bool);
      void trackSelectionChanged();
      void trackInfoScroll(int);
      
      //void iNameChanged();
      ///void iInputChannelChanged(const QString&);
      void iOutputChannelChanged(int);
      ///void iInputPortChanged(const QString&);
      void iOutputPortChanged(int);
      void iProgHBankChanged();
      void iProgLBankChanged();
      void iProgramChanged();
      void iProgramDoubleClicked();
      void iLautstChanged(int);
      void iLautstDoubleClicked();
      void iTranspChanged(int);
      void iAnschlChanged(int);
      void iVerzChanged(int);
      void iLenChanged(int);
      void iKomprChanged(int);
      void iPanChanged(int);
      void iPanDoubleClicked();
      void songChanged(int);
      void modeChange(int);
      void instrPopup();
      void setTime(unsigned);
      void headerMoved();
      void globalPitchChanged(int);
      void globalTempoChanged(int);
      void setTempo50();
      void setTempo100();
      void setTempo200();
      //void seek();
      void recordClicked();
      void progRecClicked();
      void volRecClicked();
      void panRecClicked();
      void recEchoToggled(bool);
      void verticalScrollSetYpos(unsigned);
      void inRoutesPressed();
      void outRoutesPressed();
      void routingPopupMenuActivated(int /*id*/);
      
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

   protected slots:
      virtual void midiTrackInfoHeartBeat();

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
      void setMode(int);
      void reset();

      void setTrackInfoLabelText();
      void setTrackInfoLabelFont();
      
      void writeStatus(int level, Xml&);
      void readStatus(Xml&);

      Track* curTrack() const { return selected; }
      void cmd(int);
      bool isSingleSelection() { return canvas->isSingleSelection(); }
      void setGlobalTempo(int);
      void clear();
      
      unsigned cursorValue() { return cursVal; }
      };

#endif

