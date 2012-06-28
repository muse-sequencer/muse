//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: arranger.cpp,v 1.33.2.21 2009/11/17 22:08:22 terminator356 Exp $
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
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

#include "config.h"

#include <stdio.h>
#include <limits.h>

#include <QComboBox>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QList>
#include <QMainWindow>
#include <QScrollBar>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QPainter>

#include "arrangerview.h"
#include "arranger.h"
#include "song.h"
#include "app.h"
#include "mtscale.h"
#include "scrollscale.h"
#include "pcanvas.h"
#include "poslabel.h"
#include "xml.h"
#include "splitter.h"
#include "lcombo.h"
#include "mtrackinfo.h"
#include "midiport.h"
#include "mididev.h"
#include "utils.h"
#include "globals.h"
#include "tlist.h"
#include "icons.h"
#include "header.h"
#include "utils.h"
#include "alayout.h"
#include "audio.h"
#include "event.h"
#include "midiseq.h"
#include "midictrl.h"
#include "mpevent.h"
#include "gconfig.h"
#include "mixer/astrip.h"
#include "spinbox.h"
#include "shortcuts.h"

namespace MusEGui {

void ScrollBar::redirectedWheelEvent(QWheelEvent* e)
{
  if(isVisible())
    wheelEvent(e);
}

std::vector<Arranger::custom_col_t> Arranger::custom_columns;     //FINDMICH TODO: eliminate all usage of new_custom_columns
std::vector<Arranger::custom_col_t> Arranger::new_custom_columns; //and instead let the arranger update without restarting muse!
QByteArray Arranger::header_state;

void Arranger::writeCustomColumns(int level, MusECore::Xml& xml)
{
  xml.tag(level++, "custom_columns");
  
  for (unsigned i=0;i<new_custom_columns.size();i++)
  {
    xml.tag(level++, "column");
    xml.strTag(level, "name", new_custom_columns[i].name);
    xml.intTag(level, "ctrl", new_custom_columns[i].ctrl);
    xml.intTag(level, "affected_pos", new_custom_columns[i].affected_pos);
    xml.etag(--level, "column");
  }
  
  xml.etag(--level, "custom_columns");
}

void Arranger::readCustomColumns(MusECore::Xml& xml)
{
      custom_columns.clear();
      
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        new_custom_columns=custom_columns;
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "column")
                              custom_columns.push_back(readOneCustomColumn(xml));
                        else
                              xml.unknown("Arranger::readCustomColumns");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "custom_columns")
                        {
                              new_custom_columns=custom_columns;
                              return;
                        }
                  default:
                        break;
                  }
            }
}

Arranger::custom_col_t Arranger::readOneCustomColumn(MusECore::Xml& xml)
{
      custom_col_t temp(0, "-");
      
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return temp;
                  case MusECore::Xml::TagStart:
                        if (tag == "name")
                              temp.name=xml.parse1();
                        else if (tag == "ctrl")
                              temp.ctrl=xml.parseInt();
                        else if (tag == "affected_pos")
                              temp.affected_pos=(custom_col_t::affected_pos_t)xml.parseInt();
                        else
                              xml.unknown("Arranger::readOneCustomColumn");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "column")
                              return temp;
                  default:
                        break;
                  }
            }
      return temp;
}



//---------------------------------------------------------
//   Arranger::setHeaderToolTips
//---------------------------------------------------------

void Arranger::setHeaderToolTips()
      {
      header->setToolTip(COL_RECORD,     tr("Enable Recording"));
      header->setToolTip(COL_MUTE,       tr("Mute/Off Indicator"));
      header->setToolTip(COL_SOLO,       tr("Solo Indicator"));
      header->setToolTip(COL_CLASS,      tr("Track Type"));
      header->setToolTip(COL_NAME,       tr("Track Name"));
      header->setToolTip(COL_OCHANNEL,   tr("Midi output channel number or audio channels"));
      header->setToolTip(COL_OPORT,      tr("Midi output port or synth midi port"));
      header->setToolTip(COL_TIMELOCK,   tr("Time Lock"));
      header->setToolTip(COL_AUTOMATION, tr("Automation parameter selection"));
      header->setToolTip(COL_CLEF,       tr("Notation clef"));
      }



//---------------------------------------------------------
//   Arranger::setHeaderWhatsThis
//---------------------------------------------------------

void Arranger::setHeaderWhatsThis()
      {
      header->setWhatsThis(COL_RECORD,   tr("Enable recording. Click to toggle."));
      header->setWhatsThis(COL_MUTE,     tr("Mute indicator. Click to toggle.\nRight-click to toggle track on/off.\nMute is designed for rapid, repeated action.\nOn/Off is not!"));
      header->setWhatsThis(COL_SOLO,     tr("Solo indicator. Click to toggle.\nConnected tracks are also 'phantom' soloed,\n indicated by a dark square."));
      header->setWhatsThis(COL_CLASS,    tr("Track type. Right-click to change\n midi and drum track types."));
      header->setWhatsThis(COL_NAME,     tr("Track name. Double-click to edit.\nRight-click for more options."));
      header->setWhatsThis(COL_OCHANNEL, tr("Midi/drum track: Output channel number.\nAudio track: Channels.\nMid/right-click to change."));
      header->setWhatsThis(COL_OPORT,    tr("Midi/drum track: Output port.\nSynth track: Assigned midi port.\nLeft-click to change.\nRight-click to show GUI."));
      header->setWhatsThis(COL_TIMELOCK, tr("Time lock"));
      header->setToolTip(COL_CLEF,       tr("Notation clef. Select this tracks notation clef."));
      }

//---------------------------------------------------------
//   Arranger
//    is the central widget in app
//---------------------------------------------------------

Arranger::Arranger(ArrangerView* parent, const char* name)
   : QWidget(parent)
      {
      setObjectName(name);
      _raster  = 0;      // measure
      selected = 0;
      showTrackinfoFlag = true;
      
      cursVal = INT_MAX;
      
      parentWin=parent;
      
      setFocusPolicy(Qt::NoFocus);
      
      //---------------------------------------------------
      //  ToolBar
      //    create toolbar in toplevel widget
      //---------------------------------------------------

      parent->addToolBarBreak();
      QToolBar* toolbar = parent->addToolBar(tr("Arranger"));
      toolbar->setObjectName("ArrangerToolbar");
      
      QLabel* label = new QLabel(tr("Cursor"));
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      toolbar->addWidget(label);
      cursorPos = new PosLabel(0);
      cursorPos->setEnabled(false);
      cursorPos->setFixedHeight(22);
      toolbar->addWidget(cursorPos);

      const char* rastval[] = {
            QT_TRANSLATE_NOOP("MusEGui::Arranger", "Off"), QT_TRANSLATE_NOOP("MusEGui::Arranger", "Bar"), "1/2", "1/4", "1/8", "1/16"
            };
      label = new QLabel(tr("Snap"));
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      toolbar->addWidget(label);
      QComboBox* raster = new QComboBox();
      for (int i = 0; i < 6; i++)
            raster->insertItem(i, tr(rastval[i]));
      raster->setCurrentIndex(1);
      // Set the audio record part snapping. Set to 0 (bar), the same as this combo box intial raster.
      MusEGlobal::song->setArrangerRaster(0);
      toolbar->addWidget(raster);
      connect(raster, SIGNAL(activated(int)), SLOT(_setRaster(int)));
      raster->setFocusPolicy(Qt::TabFocus);

      // Song len
      label = new QLabel(tr("Len"));
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      toolbar->addWidget(label);

      // song length is limited to 10000 bars; the real song len is limited
      // by overflows in tick computations
      lenEntry = new SpinBox(1, 10000, 1);
      lenEntry->setFocusPolicy(Qt::StrongFocus);
      lenEntry->setValue(MusEGlobal::song->len());
      lenEntry->setToolTip(tr("song length - bars"));
      lenEntry->setWhatsThis(tr("song length - bars"));
      toolbar->addWidget(lenEntry);
      connect(lenEntry, SIGNAL(valueChanged(int)), SLOT(songlenChanged(int)));

      typeBox = new LabelCombo(tr("Type"), 0);
      typeBox->insertItem(0, tr("NO"));
      typeBox->insertItem(1, tr("GM"));
      typeBox->insertItem(2, tr("GS"));
      typeBox->insertItem(3, tr("XG"));
      typeBox->setCurrentIndex(0);
      typeBox->setToolTip(tr("midi song type"));
      typeBox->setWhatsThis(tr("midi song type"));
      typeBox->setFocusPolicy(Qt::TabFocus);
      toolbar->addWidget(typeBox);
      connect(typeBox, SIGNAL(activated(int)), SLOT(modeChange(int)));

      label = new QLabel(tr("Pitch"));
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      toolbar->addWidget(label);
      
      globalPitchSpinBox = new SpinBox(-127, 127, 1);
      globalPitchSpinBox->setFocusPolicy(Qt::StrongFocus);
      globalPitchSpinBox->setValue(MusEGlobal::song->globalPitchShift());
      globalPitchSpinBox->setToolTip(tr("midi pitch"));
      globalPitchSpinBox->setWhatsThis(tr("global midi pitch shift"));
      toolbar->addWidget(globalPitchSpinBox);
      connect(globalPitchSpinBox, SIGNAL(valueChanged(int)), SLOT(globalPitchChanged(int)));
      
      label = new QLabel(tr("Tempo"));
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      toolbar->addWidget(label);
      
      globalTempoSpinBox = new SpinBox(50, 200, 1, toolbar);
      globalTempoSpinBox->setFocusPolicy(Qt::StrongFocus);
      globalTempoSpinBox->setSuffix(QString("%"));
      globalTempoSpinBox->setValue(MusEGlobal::tempomap.globalTempo());
      globalTempoSpinBox->setToolTip(tr("midi tempo"));
      globalTempoSpinBox->setWhatsThis(tr("midi tempo"));
      toolbar->addWidget(globalTempoSpinBox);
      connect(globalTempoSpinBox, SIGNAL(valueChanged(int)), SLOT(globalTempoChanged(int)));
      
      QToolButton* tempo50  = new QToolButton();
      tempo50->setText(QString("50%"));
      tempo50->setFocusPolicy(Qt::NoFocus);
      toolbar->addWidget(tempo50);
      connect(tempo50, SIGNAL(clicked()), SLOT(setTempo50()));
      
      QToolButton* tempo100 = new QToolButton();
      tempo100->setText(tr("N"));
      tempo100->setFocusPolicy(Qt::NoFocus);
      toolbar->addWidget(tempo100);
      connect(tempo100, SIGNAL(clicked()), SLOT(setTempo100()));
      
      QToolButton* tempo200 = new QToolButton();
      tempo200->setText(QString("200%"));
      tempo200->setFocusPolicy(Qt::NoFocus);
      toolbar->addWidget(tempo200);
      connect(tempo200, SIGNAL(clicked()), SLOT(setTempo200()));

      QVBoxLayout* box  = new QVBoxLayout(this);
      box->setContentsMargins(0, 0, 0, 0);
      box->setSpacing(0);
      box->addWidget(MusECore::hLine(this), Qt::AlignTop);

      //---------------------------------------------------
      //  Tracklist
      //---------------------------------------------------

      int xscale = -100;
      int yscale = 1;

      split  = new Splitter(Qt::Horizontal, this, "split");
      split->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
      box->addWidget(split, 1000);

      tracklist = new QWidget(split);

      split->setStretchFactor(split->indexOf(tracklist), 0);
      QSizePolicy tpolicy = QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
      tpolicy.setHorizontalStretch(0);
      tpolicy.setVerticalStretch(100);
      tracklist->setSizePolicy(tpolicy);

      QWidget* editor = new QWidget(split);
      split->setStretchFactor(split->indexOf(editor), 1);
      QSizePolicy epolicy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      epolicy.setHorizontalStretch(255);
      epolicy.setVerticalStretch(100);
      editor->setSizePolicy(epolicy);

      //---------------------------------------------------
      //    Track Info
      //---------------------------------------------------

      infoScroll = new ScrollBar(Qt::Vertical, tracklist);
      infoScroll->setObjectName("infoScrollBar");
      //genTrackInfo(tracklist); // Moved below

      // Track-Info Button
      ib  = new QToolButton(tracklist);
      ib->setText(tr("TrackInfo"));
      ib->setCheckable(true);
      ib->setChecked(showTrackinfoFlag);
      ib->setFocusPolicy(Qt::NoFocus);
      connect(ib, SIGNAL(toggled(bool)), SLOT(showTrackInfo(bool)));
      
      // set up the header
      header = new Header(tracklist, "header");
      header->setFixedHeight(30);

      QFontMetrics fm1(header->font());
      int fw = 8;

      header->setColumnLabel(tr("R"), COL_RECORD, fm1.width('R')+fw);
      header->setColumnLabel(tr("M"), COL_MUTE, fm1.width('M')+fw);
      header->setColumnLabel(tr("S"), COL_SOLO, fm1.width('S')+fw);
      header->setColumnLabel(tr("C"), COL_CLASS, fm1.width('C')+fw);
      header->setColumnLabel(tr("Track"), COL_NAME, 100);
      header->setColumnLabel(tr("Port"), COL_OPORT, 60);
      header->setColumnLabel(tr("Ch"), COL_OCHANNEL, 30);
      header->setColumnLabel(tr("T"), COL_TIMELOCK, fm1.width('T')+fw);
      header->setColumnLabel(tr("Automation"), COL_AUTOMATION, 75);
      header->setColumnLabel(tr("Clef"), COL_CLEF, 75);
      for (unsigned i=0;i<custom_columns.size();i++)
        header->setColumnLabel(custom_columns[i].name, COL_CUSTOM_MIDICTRL_OFFSET+i, MAX(fm1.width(custom_columns[i].name)+fw, 30));
      header->setResizeMode(COL_RECORD, QHeaderView::Fixed);
      header->setResizeMode(COL_MUTE, QHeaderView::Fixed);
      header->setResizeMode(COL_SOLO, QHeaderView::Fixed);
      header->setResizeMode(COL_CLASS, QHeaderView::Fixed);
      header->setResizeMode(COL_NAME, QHeaderView::Interactive);
      header->setResizeMode(COL_OPORT, QHeaderView::Interactive);
      header->setResizeMode(COL_OCHANNEL, QHeaderView::Fixed);
      header->setResizeMode(COL_TIMELOCK, QHeaderView::Fixed);
      header->setResizeMode(COL_AUTOMATION, QHeaderView::Interactive);
      header->setResizeMode(COL_CLEF, QHeaderView::Interactive);
      for (unsigned i=0;i<custom_columns.size();i++)
        header->setResizeMode(COL_CUSTOM_MIDICTRL_OFFSET+i, QHeaderView::Interactive);

      setHeaderToolTips();
      setHeaderWhatsThis();
      header->setMovable (true);
      header->restoreState(header_state);


      list = new TList(header, tracklist, "tracklist");
      
      // Do this now that the list is available.
      genTrackInfo(tracklist);
      
      connect(list, SIGNAL(selectionChanged(MusECore::Track*)), SLOT(trackSelectionChanged()));
      connect(list, SIGNAL(selectionChanged(MusECore::Track*)), midiTrackInfo, SLOT(setTrack(MusECore::Track*)));
      connect(header, SIGNAL(sectionResized(int,int,int)), list, SLOT(redraw()));
      connect(header, SIGNAL(sectionMoved(int,int,int)), list, SLOT(redraw()));

      //  tracklist:
      //
      //         0         1         2
      //   +-----------+--------+---------+
      //   | Trackinfo | scroll | Header  | 0
      //   |           | bar    +---------+
      //   |           |        | TList   | 1
      //   +-----------+--------+---------+
      //   |             hline            | 2
      //   +-----+------------------------+
      //   | ib  |                        | 3
      //   +-----+------------------------+

      connect(infoScroll, SIGNAL(valueChanged(int)), SLOT(trackInfoScroll(int)));
      tgrid  = new TLLayout(tracklist); // layout manager for this
      tgrid->wadd(0, trackInfo);
      tgrid->wadd(1, infoScroll);
      tgrid->wadd(2, header);
      tgrid->wadd(3, list);
      tgrid->wadd(4, MusECore::hLine(tracklist));
      tgrid->wadd(5, ib);

      //---------------------------------------------------
      //    Editor
      //---------------------------------------------------

      int offset = AL::sigmap.ticksMeasure(0);
      hscroll = new ScrollScale(-1000, -10, xscale, MusEGlobal::song->len(), Qt::Horizontal, editor, -offset);
      hscroll->setFocusPolicy(Qt::NoFocus);
      ib->setFixedHeight(hscroll->sizeHint().height());

      vscroll = new QScrollBar(editor);
      vscroll->setMinimum(0);
      vscroll->setMaximum(20*20);
      vscroll->setSingleStep(5);
      vscroll->setPageStep(25); // FIXME: too small steps here for me (flo), better control via window height?
      vscroll->setValue(0);
      vscroll->setOrientation(Qt::Vertical);      

      list->setScroll(vscroll);

      QList<int> vallist;
      vallist.append(tgrid->maximumSize().width());
      split->setSizes(vallist);

      QGridLayout* egrid  = new QGridLayout(editor);
      egrid->setColumnStretch(0, 50);
      egrid->setRowStretch(2, 50);
      egrid->setContentsMargins(0, 0, 0, 0);  
      egrid->setSpacing(0);  

      time = new MTScale(&_raster, editor, xscale);
      time->setOrigin(-offset, 0);
      canvas = new PartCanvas(&_raster, editor, xscale, yscale);
      canvas->setBg(MusEGlobal::config.partCanvasBg);
      canvas->setCanvasTools(arrangerTools);
      canvas->setOrigin(-offset, 0);
      canvas->setFocus();

      list->setFocusProxy(canvas); // Make it easy for track list popup line editor to give focus back to canvas.

      connect(canvas, SIGNAL(setUsedTool(int)), this, SIGNAL(setUsedTool(int)));
      connect(canvas, SIGNAL(trackChanged(MusECore::Track*)), list, SLOT(selectTrack(MusECore::Track*)));
      connect(list, SIGNAL(keyPressExt(QKeyEvent*)), canvas, SLOT(redirKeypress(QKeyEvent*)));
      connect(canvas, SIGNAL(selectTrackAbove()), list, SLOT(selectTrackAbove()));
      connect(canvas, SIGNAL(selectTrackBelow()), list, SLOT(selectTrackBelow()));
      connect(canvas, SIGNAL(horizontalZoomIn()), SLOT(horizontalZoomIn()));
      connect(canvas, SIGNAL(horizontalZoomOut()), SLOT(horizontalZoomOut()));
      connect(lenEntry,           SIGNAL(returnPressed()), SLOT(focusCanvas()));
      connect(lenEntry,           SIGNAL(escapePressed()), SLOT(focusCanvas()));
      connect(globalPitchSpinBox, SIGNAL(returnPressed()), SLOT(focusCanvas()));
      connect(globalPitchSpinBox, SIGNAL(escapePressed()), SLOT(focusCanvas()));
      connect(globalTempoSpinBox, SIGNAL(returnPressed()), SLOT(focusCanvas()));
      connect(globalTempoSpinBox, SIGNAL(escapePressed()), SLOT(focusCanvas()));
      connect(midiTrackInfo,      SIGNAL(returnPressed()), SLOT(focusCanvas()));
      connect(midiTrackInfo,      SIGNAL(escapePressed()), SLOT(focusCanvas()));
      
      //connect(this,      SIGNAL(redirectWheelEvent(QWheelEvent*)), canvas, SLOT(redirectedWheelEvent(QWheelEvent*)));
      connect(list,      SIGNAL(redirectWheelEvent(QWheelEvent*)), canvas, SLOT(redirectedWheelEvent(QWheelEvent*)));
      connect(trackInfo, SIGNAL(redirectWheelEvent(QWheelEvent*)), infoScroll, SLOT(redirectedWheelEvent(QWheelEvent*)));
      
      egrid->addWidget(time, 0, 0, 1, 2);
      egrid->addWidget(MusECore::hLine(editor), 1, 0, 1, 2);

      egrid->addWidget(canvas,  2, 0);
      egrid->addWidget(vscroll, 2, 1);
      egrid->addWidget(hscroll, 3, 0, Qt::AlignBottom);

      connect(vscroll, SIGNAL(valueChanged(int)), canvas, SLOT(setYPos(int)));
      connect(hscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)),  canvas, SLOT(setXMag(int)));
      connect(vscroll, SIGNAL(valueChanged(int)), list,   SLOT(setYPos(int)));
      connect(hscroll, SIGNAL(scrollChanged(int)), time,   SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)),  time,   SLOT(setXMag(int)));
      connect(canvas,  SIGNAL(timeChanged(unsigned)),   SLOT(setTime(unsigned)));
      connect(canvas,  SIGNAL(verticalScroll(unsigned)),SLOT(verticalScrollSetYpos(unsigned)));
      connect(canvas,  SIGNAL(horizontalScroll(unsigned)),hscroll, SLOT(setPos(unsigned)));
      connect(canvas,  SIGNAL(horizontalScrollNoLimit(unsigned)),hscroll, SLOT(setPosNoLimit(unsigned))); 
      connect(time,    SIGNAL(timeChanged(unsigned)),   SLOT(setTime(unsigned)));

      connect(canvas, SIGNAL(tracklistChanged()), list, SLOT(tracklistChanged()));
      connect(canvas, SIGNAL(dclickPart(MusECore::Track*)), SIGNAL(editPart(MusECore::Track*)));
      connect(canvas, SIGNAL(startEditor(MusECore::PartList*,int)),   SIGNAL(startEditor(MusECore::PartList*, int)));

      connect(MusEGlobal::song,   SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(canvas, SIGNAL(followEvent(int)), hscroll, SLOT(setOffset(int)));
      connect(canvas, SIGNAL(selectionChanged()), SIGNAL(selectionChanged()));
      connect(canvas, SIGNAL(dropSongFile(const QString&)), SIGNAL(dropSongFile(const QString&)));
      connect(canvas, SIGNAL(dropMidiFile(const QString&)), SIGNAL(dropMidiFile(const QString&)));

      connect(canvas, SIGNAL(toolChanged(int)), SIGNAL(toolChanged(int)));
      connect(MusEGlobal::song,   SIGNAL(controllerChanged(MusECore::Track*, int)), SLOT(controllerChanged(MusECore::Track*, int)));

      configChanged();  // set configuration values
      if(canvas->part())
        midiTrackInfo->setTrack(canvas->part()->track());   
      showTrackInfo(showTrackinfoFlag);
      
      // Take care of some tabbies!
      setTabOrder(tempo200, trackInfo);
      setTabOrder(trackInfo, infoScroll);
      setTabOrder(infoScroll, list);
      setTabOrder(list, canvas);
      //setTabOrder(canvas, ib);
      //setTabOrder(ib, hscroll);
      }

// DELETETHIS 20
//---------------------------------------------------------
//   updateHScrollRange
//---------------------------------------------------------

//void Arranger::updateHScrollRange()
//{
//      int s = 0, e = MusEGlobal::song->len();
      // Show one more measure.
//      e += AL::sigmap.ticksMeasure(e);  
      // Show another quarter measure due to imprecise drawing at canvas end point.
//      e += AL::sigmap.ticksMeasure(e) / 4;
      // Compensate for the fixed vscroll width. 
//      e += canvas->rmapxDev(-vscroll->width()); 
//      int s1, e1;
//      hscroll->range(&s1, &e1);
//      if(s != s1 || e != e1) 
//        hscroll->setRange(s, e);
//}


//---------------------------------------------------------
//   setTime
//---------------------------------------------------------

void Arranger::setTime(unsigned tick)
      {
      if (tick == INT_MAX)
            cursorPos->setEnabled(false);
      else {
            cursVal = tick;
            cursorPos->setEnabled(true);
            cursorPos->setValue(tick);
            time->setPos(3, tick, false);
            }
      }

//---------------------------------------------------------
//   toolChange
//---------------------------------------------------------

void Arranger::setTool(int t)
      {
      canvas->setTool(t);
      }

//---------------------------------------------------------
//   dclickPart
//---------------------------------------------------------

void Arranger::dclickPart(MusECore::Track* t)
      {
      emit editPart(t);
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void Arranger::configChanged()
      {
      if (MusEGlobal::config.canvasBgPixmap.isEmpty()) {
            canvas->setBg(MusEGlobal::config.partCanvasBg);
            canvas->setBg(QPixmap());
      }
      else {
            canvas->setBg(QPixmap(MusEGlobal::config.canvasBgPixmap));
      }
      }

//---------------------------------------------------------
//   focusCanvas
//---------------------------------------------------------

void Arranger::focusCanvas()
{
  if(MusEGlobal::config.smartFocus)
  {
    canvas->setFocus();
    canvas->activateWindow();
  }
}

//---------------------------------------------------------
//   songlenChanged
//---------------------------------------------------------

void Arranger::songlenChanged(int n)
      {
      int newLen = AL::sigmap.bar2tick(n, 0, 0);
      MusEGlobal::song->setLen(newLen);
      }
//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void Arranger::songChanged(int type)
      {
      // Is it simply a midi controller value adjustment? Forget it.
      if(type != SC_MIDI_CONTROLLER)
      {
        // TEST p4.0.36 Try these, may need more/less. 
        if(type & ( SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED | 
           SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED))  
        {
          unsigned endTick = MusEGlobal::song->len();
          int offset  = AL::sigmap.ticksMeasure(endTick);
          hscroll->setRange(-offset, endTick + offset);  //DEBUG
          canvas->setOrigin(-offset, 0);
          time->setOrigin(-offset, 0);
    
          int bar, beat;
          unsigned tick;
          AL::sigmap.tickValues(endTick, &bar, &beat, &tick);
          if (tick || beat)
                ++bar;
          lenEntry->blockSignals(true);
          lenEntry->setValue(bar);
          lenEntry->blockSignals(false);
        }
        
        if(type & SC_SONG_TYPE)    
          setMode(MusEGlobal::song->mtype());
          
        if(type & (SC_SELECTION | SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED))
          trackSelectionChanged();
        
        // Keep this light, partsChanged is a heavy move!       TEST p4.0.36 Try these, may need more.
        if(type & (SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED | 
                   SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED | 
                   SC_SIG | SC_TEMPO | SC_MASTER)) // Maybe sig. Requires tempo.
          canvas->partsChanged();
        
        if (type & SC_SIG)
              time->redraw();
        if (type & SC_TEMPO)
              setGlobalTempo(MusEGlobal::tempomap.globalTempo());
              
        if(type & SC_TRACK_REMOVED)
        {
          AudioStrip* w = (AudioStrip*)(trackInfo->getWidget(2));
          if(w)
          {
            MusECore::Track* t = w->getTrack();
            if(t)
            {
              MusECore::TrackList* tl = MusEGlobal::song->tracks();
              MusECore::iTrack it = tl->find(t);
              if(it == tl->end())
              {
                delete w;
                trackInfo->addWidget(0, 2);
                //trackInfo->insertWidget(2, 0);
                selected = 0;
              } 
            }   
          } 
        }
        
        // TEST p4.0.36 Try this DELETETHIS and below and even more below
        if(type & ( //SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED | 
           SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED | 
           SC_EVENT_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED)) //|
           //SC_SIG | SC_TEMPO))  // Maybe sig. and tempo. No, moved above.
        canvas->redraw();
        
      }
            
      updateTrackInfo(type);
    }

//---------------------------------------------------------
//   trackSelectionChanged
//---------------------------------------------------------

void Arranger::trackSelectionChanged()
      {
      MusECore::TrackList* tracks = MusEGlobal::song->tracks();
      MusECore::Track* track = 0;
      for (MusECore::iTrack t = tracks->begin(); t != tracks->end(); ++t) {
            if ((*t)->selected()) {
                  track = *t;
                  break;
                  }
            }
      if (track == selected)
            return;
      selected = track;
      updateTrackInfo(-1);
      }

//---------------------------------------------------------
//   modeChange
//---------------------------------------------------------

void Arranger::modeChange(int mode)
      {
      MusEGlobal::song->setMType(MType(mode));
      updateTrackInfo(-1);
      focusCanvas();
      }

//---------------------------------------------------------
//   setMode
//---------------------------------------------------------

void Arranger::setMode(int mode)
      {
      typeBox->blockSignals(true);
      // This will only set if different.
      typeBox->setCurrentIndex(mode);
      typeBox->blockSignals(false);
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void Arranger::writeStatus(int level, MusECore::Xml& xml)
      {
      xml.tag(level++, "arranger");
      xml.intTag(level, "info", ib->isChecked());
      split->writeStatus(level, xml);

      xml.intTag(level, "xpos", hscroll->pos());
      xml.intTag(level, "xmag", hscroll->mag());
      xml.intTag(level, "ypos", vscroll->value());
      xml.etag(level, "arranger");
      }

void Arranger::writeConfiguration(int level, MusECore::Xml& xml)
      {
      xml.tag(level++, "arranger");
      writeCustomColumns(level, xml);
      xml.strTag(level, "tlist_header", header->saveState().toHex().constData());
      xml.etag(level, "arranger");
      }

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void Arranger::readConfiguration(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "tlist_header")
                              header_state = QByteArray::fromHex(xml.parse1().toAscii());
                        else if (tag == "custom_columns")
                              readCustomColumns(xml);
                        else
                              xml.unknown("Arranger");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "arranger")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void Arranger::readStatus(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "info")
                              showTrackinfoFlag = xml.parseInt();
                        else if (tag == split->objectName())
                              split->readStatus(xml);
                        else if (tag == "xmag")
                              hscroll->setMag(xml.parseInt());
                        else if (tag == "xpos") {
                              int hpos = xml.parseInt();
                              hscroll->setPos(hpos);
                              }
                        else if (tag == "ypos")
                              vscroll->setValue(xml.parseInt());
                        else
                              xml.unknown("Arranger");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "arranger") {
                              ib->setChecked(showTrackinfoFlag);
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void Arranger::_setRaster(int index)
      {
      static int rasterTable[] = {
            1, 0, 768, 384, 192, 96
            };
      _raster = rasterTable[index];
      // Set the audio record part snapping.
      MusEGlobal::song->setArrangerRaster(_raster);
      canvas->redraw();
      focusCanvas();
      }

//---------------------------------------------------------
//   reset
//---------------------------------------------------------

void Arranger::reset()
      {
      canvas->setXPos(0);
      canvas->setYPos(0);
      hscroll->setPos(0);
      vscroll->setValue(0);
      time->setXPos(0);
      time->setYPos(0);
      }

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void Arranger::cmd(int cmd)
      {
      int ncmd;
      switch (cmd) {
            case CMD_CUT_PART:
                  ncmd = PartCanvas::CMD_CUT_PART;
                  break;
            case CMD_COPY_PART:
                  ncmd = PartCanvas::CMD_COPY_PART;
                  break;
            case CMD_COPY_PART_IN_RANGE:
                  ncmd = PartCanvas::CMD_COPY_PART_IN_RANGE;
                  break;
            case CMD_PASTE_PART:
                  ncmd = PartCanvas::CMD_PASTE_PART;
                  break;
            case CMD_PASTE_CLONE_PART:
                  ncmd = PartCanvas::CMD_PASTE_CLONE_PART;
                  break;
            case CMD_PASTE_DIALOG:
                  ncmd = PartCanvas::CMD_PASTE_DIALOG;
                  break;
            case CMD_PASTE_CLONE_DIALOG:
                  ncmd = PartCanvas::CMD_PASTE_CLONE_DIALOG;
                  break;
            case CMD_INSERT_EMPTYMEAS:
                  ncmd = PartCanvas::CMD_INSERT_EMPTYMEAS;
                  break;
            default:
                  return;
            }
      canvas->cmd(ncmd);
      }

//---------------------------------------------------------
//   globalPitchChanged
//---------------------------------------------------------

void Arranger::globalPitchChanged(int val)
      {
      MusEGlobal::song->setGlobalPitchShift(val);
      }

//---------------------------------------------------------
//   globalTempoChanged
//---------------------------------------------------------

void Arranger::globalTempoChanged(int val)
      {
      MusEGlobal::audio->msgSetGlobalTempo(val);
      MusEGlobal::song->tempoChanged();
      }

//---------------------------------------------------------
//   setTempo50
//---------------------------------------------------------

void Arranger::setTempo50()
      {
      setGlobalTempo(50);
      }

//---------------------------------------------------------
//   setTempo100
//---------------------------------------------------------

void Arranger::setTempo100()
      {
      setGlobalTempo(100);
      }

//---------------------------------------------------------
//   setTempo200
//---------------------------------------------------------

void Arranger::setTempo200()
      {
      setGlobalTempo(200);
      }

//---------------------------------------------------------
//   setGlobalTempo
//---------------------------------------------------------

void Arranger::setGlobalTempo(int val)
      {
      if(val != globalTempoSpinBox->value())
        globalTempoSpinBox->setValue(val);
      }

//---------------------------------------------------------
//   verticalScrollSetYpos
//---------------------------------------------------------
void Arranger::verticalScrollSetYpos(unsigned ypos)
      {
      vscroll->setValue(ypos);
      }

//---------------------------------------------------------
//   trackInfoScroll
//---------------------------------------------------------

void Arranger::trackInfoScroll(int y)
      {
      if (trackInfo->visibleWidget())
            trackInfo->visibleWidget()->move(0, -y);
      }

//---------------------------------------------------------
//   WidgetStack
//---------------------------------------------------------

WidgetStack::WidgetStack(QWidget* parent, const char* name)
   : QWidget(parent)
      {
      setObjectName(name);
      top = -1;
      }

//---------------------------------------------------------
//   raiseWidget
//---------------------------------------------------------

void WidgetStack::raiseWidget(int idx)
      {
      if (top != -1) {
            if (stack[top])
                  stack[top]->hide();
            }
      top = idx;
      if (idx == -1)
            return;
      int n = stack.size();
      if (idx >= n)
            return;
      if (stack[idx])
            stack[idx]->show();
      }

//---------------------------------------------------------
//   addWidget
//---------------------------------------------------------

void WidgetStack::addWidget(QWidget* w, unsigned int n)
      {
      if (w)
            w->hide();
      if (stack.size() <= n )
            stack.push_back(w);
      else
            stack[n] = w;
      }

QWidget* WidgetStack::getWidget(unsigned int n)
      {
      if (stack.size() <= n )
            return 0;
      return stack[n];
      }

//---------------------------------------------------------
//   visibleWidget
//---------------------------------------------------------

QWidget* WidgetStack::visibleWidget() const
      {
      if (top != -1)
            return stack[top];
      return 0;
      }

//---------------------------------------------------------
//   minimumSizeHint
//---------------------------------------------------------

QSize WidgetStack::minimumSizeHint() const
      {
      if (top == -1)
            return (QSize(0, 0));

      QSize s(0,0);
      for (unsigned int i = 0; i < stack.size(); ++i) {
            if (stack[i]) {
                  QSize ss = stack[i]->minimumSizeHint();
                  if (!ss.isValid())
                        ss = stack[i]->minimumSize();
                  s = s.expandedTo(ss);
                  }
            }

      return s;
      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void WidgetStack::wheelEvent(QWheelEvent* ev)
      {
      emit redirectWheelEvent(ev);
      }

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void Arranger::clear()
      {
      AudioStrip* w = (AudioStrip*)(trackInfo->getWidget(2));
      if (w)
            delete w;
      trackInfo->addWidget(0, 2);
      selected = 0;
      midiTrackInfo->setTrack(0);
      }

//void Arranger::wheelEvent(QWheelEvent* ev)
//      {
//      emit redirectWheelEvent(ev);
//      }

void Arranger::controllerChanged(MusECore::Track *t, int ctrlId)
{
      canvas->controllerChanged(t, ctrlId);
}

//---------------------------------------------------------
//   showTrackInfo
//---------------------------------------------------------

void Arranger::showTrackInfo(bool flag)
      {
      showTrackinfoFlag = flag;
      trackInfo->setVisible(flag);
      infoScroll->setVisible(flag);
      updateTrackInfo(-1);
      }

//---------------------------------------------------------
//   genTrackInfo
//---------------------------------------------------------

void Arranger::genTrackInfo(QWidget* parent)
      {
      trackInfo = new WidgetStack(parent, "trackInfoStack");

      noTrackInfo          = new QWidget(trackInfo);
      noTrackInfo->setAutoFillBackground(true);
      QPixmap *noInfoPix   = new QPixmap(160, 1000);
      const QPixmap *logo  = new QPixmap(*museLeftSideLogo);
      noInfoPix->fill(noTrackInfo->palette().color(QPalette::Window) );
      QPainter p(noInfoPix);
      p.drawPixmap(10, 0, *logo, 0,0, logo->width(), logo->height());

      QPalette palette;
      palette.setBrush(noTrackInfo->backgroundRole(), QBrush(*noInfoPix));
      noTrackInfo->setPalette(palette);
      noTrackInfo->setGeometry(0, 0, 65, 200);
      noTrackInfo->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));

      midiTrackInfo = new MidiTrackInfo(trackInfo);
      
      trackInfo->addWidget(noTrackInfo,   0);
      trackInfo->addWidget(midiTrackInfo, 1);
      trackInfo->addWidget(0, 2);
      }

//---------------------------------------------------------
//   updateTrackInfo
//---------------------------------------------------------

void Arranger::updateTrackInfo(int flags)
      {
      if (!showTrackinfoFlag) {
            switchInfo(-1);
            return;
            }
      if (selected == 0) {
            switchInfo(0);
            return;
            }
      if (selected->isMidiTrack()) {
            switchInfo(1);
            // If a different part was selected
            if(midiTrackInfo->track() != selected)
              // Set a new track and do a complete update.
              midiTrackInfo->setTrack(selected);
            else  
              // Otherwise just regular update with specific flags.
              midiTrackInfo->updateTrackInfo(flags);
            }
      else {
            switchInfo(2);
            }
      }

//---------------------------------------------------------
//   switchInfo
//---------------------------------------------------------

void Arranger::switchInfo(int n)
      {
      if (n == 2) {
            AudioStrip* w = (AudioStrip*)(trackInfo->getWidget(2));
            if (w == 0 || selected != w->getTrack()) {
                  if (w)
                        delete w;
                  w = new AudioStrip(trackInfo, (MusECore::AudioTrack*)selected);
                  //w->setFocusPolicy(Qt::TabFocus);  // p4.0.9
                  connect(MusEGlobal::song, SIGNAL(songChanged(int)), w, SLOT(songChanged(int)));
                  connect(MusEGlobal::muse, SIGNAL(configChanged()), w, SLOT(configChanged()));
                  w->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
                  trackInfo->addWidget(w, 2);
                  w->show();
                  //setTabOrder(midiTrackInfo, w); // p4.0.9
                  tgrid->activate();
                  tgrid->update();   // muse-2 Qt4
                  }
            }
      if (trackInfo->curIdx() == n)
            return;
      trackInfo->raiseWidget(n);
      tgrid->activate();
      tgrid->update();   // muse-2 Qt4
      }

/* DELETETHIS 12
QSize WidgetStack::minimumSize() const 
{ 
  printf("WidgetStack::minimumSize\n");  
  return minimumSizeHint(); 
}

int WidgetStack::minimumHeight() const 
{ 
  printf("WidgetStack::minimumHeight\n");  
  return minimumSizeHint().height(); 
}
*/

void Arranger::keyPressEvent(QKeyEvent* event)
{
  int key = event->key();
  if (((QInputEvent*)event)->modifiers() & Qt::ShiftModifier)
        key += Qt::SHIFT;
  if (((QInputEvent*)event)->modifiers() & Qt::AltModifier)
        key += Qt::ALT;
  if (((QInputEvent*)event)->modifiers() & Qt::ControlModifier)
        key+= Qt::CTRL;

  if (key == shortcuts[SHRT_ZOOM_IN].key) {
        horizontalZoomIn();
        return;
        }
  else if (key == shortcuts[SHRT_ZOOM_OUT].key) {
        horizontalZoomOut();
        return;
        }

  QWidget::keyPressEvent(event);
}

void Arranger::horizontalZoomIn()
{
  int mag = hscroll->mag();
  int zoomlvl = ScrollScale::getQuickZoomLevel(mag);
  if (zoomlvl < 23)
        zoomlvl++;

  int newmag = ScrollScale::convertQuickZoomLevelToMag(zoomlvl);

  hscroll->setMag(newmag);

}

void Arranger::horizontalZoomOut()
{
  int mag = hscroll->mag();
  int zoomlvl = ScrollScale::getQuickZoomLevel(mag);
  if (zoomlvl > 1)
        zoomlvl--;

  int newmag = ScrollScale::convertQuickZoomLevelToMag(zoomlvl);

  hscroll->setMag(newmag);

}

} // namespace MusEGui
