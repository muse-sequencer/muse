//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: arranger.cpp,v 1.33.2.21 2009/11/17 22:08:22 terminator356 Exp $
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
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
#include "muse_math.h"

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
#include <QCursor>
#include <QPoint>
#include <QRect>

#include "arrangerview.h"
#include "arranger.h"
#include "song.h"
#include "app.h"
#include "mtscale.h"
#include "scrollscale.h"
#include "scrollbar.h"
#include "pcanvas.h"
#include "poslabel.h"
#include "xml.h"
#include "splitter.h"
#include "lcombo.h"
#include "midiport.h"
#include "mididev.h"
#include "utils.h"
#include "globals.h"
#include "tlist.h"
#include "icons.h"
#include "header.h"
#include "utils.h"
#include "widget_stack.h"
#include "trackinfo_layout.h"
#include "audio.h"
#include "event.h"
#include "midiseq.h"
#include "midictrl.h"
#include "mpevent.h"
#include "gconfig.h"
#include "mixer/astrip.h"
#include "mixer/mstrip.h"
#include "spinbox.h"
#include "shortcuts.h"
#include "ttoolbutton.h"

namespace MusEGui {

std::vector<Arranger::custom_col_t> Arranger::custom_columns;     //FINDMICH TODO: eliminate all usage of new_custom_columns
std::vector<Arranger::custom_col_t> Arranger::new_custom_columns; //and instead let the arranger update without restarting muse!
QByteArray Arranger::header_state;
static const char* gArrangerRasterStrings[] = {
      QT_TRANSLATE_NOOP("MusEGui::Arranger", "Off"), QT_TRANSLATE_NOOP("MusEGui::Arranger", "Bar"), "1/2", "1/4", "1/8", "1/16"
      };
static int gArrangerRasterTable[] = { 1, 0, 768, 384, 192, 96 };

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
      header->setToolTip(COL_TRACK_IDX,  tr("Track index"));
      header->setToolTip(COL_INPUT_MONITOR, tr("Enable input monitor"));
      header->setToolTip(COL_RECORD,     tr("Enable recording"));
      header->setToolTip(COL_MUTE,       tr("Mute/Off indicator"));
      header->setToolTip(COL_SOLO,       tr("Solo indicator"));
      header->setToolTip(COL_CLASS,      tr("Track type"));
      header->setToolTip(COL_NAME,       tr("Track name"));
      header->setToolTip(COL_OCHANNEL,   tr("Midi output channel number or audio channels"));
      header->setToolTip(COL_OPORT,      tr("Midi output port or synth midi port"));
      header->setToolTip(COL_TIMELOCK,   tr("Time lock"));
      header->setToolTip(COL_AUTOMATION, tr("Automation parameter selection"));
      header->setToolTip(COL_CLEF,       tr("Notation clef"));
      }



//---------------------------------------------------------
//   Arranger::setHeaderWhatsThis
//---------------------------------------------------------

void Arranger::setHeaderWhatsThis()
      {
      header->setWhatsThis(COL_TRACK_IDX, tr("Track index"));
      header->setWhatsThis(COL_INPUT_MONITOR, tr("Enable input monitor. Click to toggle.\nPasses input through to output for monitoring.\n"
                                                 "See also Settings: Automatically Monitor On Record Arm."));
      header->setWhatsThis(COL_RECORD,   tr("Enable recording. Click to toggle.\n"
                                            "See also Settings: Automatically Monitor On Record Arm."));
      header->setWhatsThis(COL_MUTE,     tr("Mute indicator. Click to toggle.\nRight-click to toggle track on/off.\nMute is designed for rapid, repeated action.\nOn/Off is not!"));
      header->setWhatsThis(COL_SOLO,     tr("Solo indicator. Click to toggle.\nConnected tracks are also 'phantom' soloed,\n indicated by a dark square."));
      header->setWhatsThis(COL_CLASS,    tr("Track type. Right-click to change\n midi and drum track types."));
      header->setWhatsThis(COL_NAME,     tr("Track name. Double-click to edit.\nRight-click for more options."));
      header->setWhatsThis(COL_OCHANNEL, tr("Midi/Drum track: Output channel number.\nAudio track: Channels.\nMid/right-click to change."));
      header->setWhatsThis(COL_OPORT,    tr("Midi/Drum track: Output port.\nSynth track: Assigned midi port.\nLeft-click to change.\nRight-click to show GUI."));
      header->setWhatsThis(COL_TIMELOCK, tr("Time lock"));
      header->setWhatsThis(COL_CLEF,     tr("Notation clef. Select this tracks notation clef."));
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
      
      _parentWin=parent;
      
      setFocusPolicy(Qt::NoFocus);
      
      //---------------------------------------------------
      //  ToolBar
      //    create toolbar in toplevel widget
      //---------------------------------------------------

      // NOTICE: Please ensure that any tool bar object names here match the names assigned 
      //          to identical or similar toolbars in class MusE or other TopWin classes. 
      //         This allows MusE::setCurrentMenuSharingTopwin() to do some magic
      //          to retain the original toolbar layout. If it finds an existing
      //          toolbar with the same object name, it /replaces/ it using insertToolBar(),
      //          instead of /appending/ with addToolBar().

      parent->addToolBarBreak();
      QToolBar* toolbar = parent->addToolBar(tr("Arranger"));
      toolbar->setObjectName("ArrangerToolbar");
      
      QLabel* label = new QLabel(tr("Cursor"));
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      toolbar->addWidget(label);
      cursorPos = new PosLabel(0);
      cursorPos->setEnabled(false);
      toolbar->addWidget(cursorPos);

      label = new QLabel(tr("Snap"));
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      toolbar->addWidget(label);
      _rasterCombo = new QComboBox();
      for (int i = 0; i < 6; i++)
            _rasterCombo->insertItem(i, tr(gArrangerRasterStrings[i]), gArrangerRasterTable[i]);
      _rasterCombo->setCurrentIndex(1);
      // Set the audio record part snapping. Set to 0 (bar), the same as this combo box initial raster.
      MusEGlobal::song->setArrangerRaster(0);
      toolbar->addWidget(_rasterCombo);
      connect(_rasterCombo, SIGNAL(activated(int)), SLOT(rasterChanged(int)));
      _rasterCombo->setFocusPolicy(Qt::TabFocus);

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
      lenEntry->setToolTip(tr("Song length - bars"));
      lenEntry->setWhatsThis(tr("Song length - bars"));
      toolbar->addWidget(lenEntry);
      connect(lenEntry, SIGNAL(valueChanged(int)), SLOT(songlenChanged(int)));

      label = new QLabel(tr("Pitch"));
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      toolbar->addWidget(label);
      
      globalPitchSpinBox = new SpinBox(-127, 127, 1);
      globalPitchSpinBox->setFocusPolicy(Qt::StrongFocus);
      globalPitchSpinBox->setValue(MusEGlobal::song->globalPitchShift());
      globalPitchSpinBox->setToolTip(tr("Midi pitch"));
      globalPitchSpinBox->setWhatsThis(tr("Global midi pitch shift"));
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
      globalTempoSpinBox->setToolTip(tr("Midi tempo"));
      globalTempoSpinBox->setWhatsThis(tr("Midi tempo"));
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

      
      trackInfoWidget = new TrackInfoWidget(split);
      split->setStretchFactor(split->indexOf(trackInfoWidget), 0);
      QSizePolicy tipolicy = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
      tipolicy.setHorizontalStretch(0);
      tipolicy.setVerticalStretch(100);
      trackInfoWidget->setSizePolicy(tipolicy);
      
      tracklist = new QWidget(split);
      split->setStretchFactor(split->indexOf(tracklist), 0);
      QSizePolicy tpolicy = QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
      tpolicy.setHorizontalStretch(0);
      tpolicy.setVerticalStretch(100);
      tracklist->setSizePolicy(tpolicy);

      editor = new QWidget(split);
      split->setStretchFactor(split->indexOf(editor), 1);
      QSizePolicy epolicy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      epolicy.setHorizontalStretch(255);
      epolicy.setVerticalStretch(100);
      editor->setSizePolicy(epolicy);

      //---------------------------------------------------
      //    Track Info
      //---------------------------------------------------

      trackInfoButton  = new CompactToolButton(this);
      trackInfoButton->setContentsMargins(0, 0, 0, 0);
      trackInfoButton->setIcon(*mixerstripSVGIcon);
      trackInfoButton->setToolTip(tr("Show mixer strip for current track"));
      trackInfoButton->setCheckable(true);
      trackInfoButton->setChecked(showTrackinfoFlag);
      trackInfoButton->setFocusPolicy(Qt::NoFocus);
      trackInfoButton->setFixedSize(52, 14);
      connect(trackInfoButton, SIGNAL(toggled(bool)), SLOT(showTrackInfo(bool)));

      genTrackInfo(trackInfoWidget);

      // set up the header
      header = new Header(tracklist, "header");
      header->setFixedHeight(31);

      header->setColumnLabel(tr("#"), COL_TRACK_IDX);
      header->setColumnIcon(*monitorOnSVGIcon, COL_INPUT_MONITOR);
      header->setColumnIcon(*recArmOnSVGIcon, COL_RECORD);
      header->setColumnIcon(*muteOnSVGIcon, COL_MUTE);
      header->setColumnIcon(*soloOnAloneSVGIcon, COL_SOLO);
      header->setColumnIcon(*tracktypeSVGIcon, COL_CLASS);
      header->setColumnLabel(tr("Track"), COL_NAME);
      header->setColumnLabel(tr("Port"), COL_OPORT);
      //: Channel
      header->setColumnLabel(tr("Ch"), COL_OCHANNEL);
      //: Time lock
      header->setColumnLabel(tr("T"), COL_TIMELOCK);
      header->setColumnLabel(tr("Automation"), COL_AUTOMATION);
      header->setColumnLabel(tr("Clef"), COL_CLEF);

      header->setSectionResizeMode(COL_TRACK_IDX, QHeaderView::Interactive);
      header->setSectionResizeMode(COL_INPUT_MONITOR, QHeaderView::Fixed);
      header->setSectionResizeMode(COL_RECORD, QHeaderView::Fixed);
      header->setSectionResizeMode(COL_MUTE, QHeaderView::Fixed);
      header->setSectionResizeMode(COL_SOLO, QHeaderView::Fixed);
      header->setSectionResizeMode(COL_CLASS, QHeaderView::Fixed);
      header->setSectionResizeMode(COL_NAME, QHeaderView::Interactive);
      header->setSectionResizeMode(COL_OPORT, QHeaderView::Interactive);
      header->setSectionResizeMode(COL_OCHANNEL, QHeaderView::Fixed);
      header->setSectionResizeMode(COL_TIMELOCK, QHeaderView::Fixed);
      header->setSectionResizeMode(COL_AUTOMATION, QHeaderView::Interactive);
      header->setSectionResizeMode(COL_CLEF, QHeaderView::Interactive);
      for (unsigned i=0;i<custom_columns.size();i++)
        header->setSectionResizeMode(COL_CUSTOM_MIDICTRL_OFFSET+i, QHeaderView::Interactive);

      // 04/18/17 Time lock remains unused. Disabled until a use is found.
      // Plans were to use it (or not) when time stretching / pitch shifting work is done.
      header->setSectionHidden(COL_TIMELOCK, true);

      setHeaderToolTips();
      setHeaderWhatsThis();
      header->setSectionsMovable (true);
      header->restoreState(header_state);

      list = new TList(header, tracklist, "tracklist");

      tlistLayout = new QVBoxLayout(tracklist);
      tlistLayout->setContentsMargins(0, 0, 0, 0);
      tlistLayout->setSpacing(0);
      tlistLayout->addWidget(header);
      tlistLayout->addWidget(list);
      
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

      //---------------------------------------------------
      //    Editor
      //---------------------------------------------------

      int offset = MusEGlobal::sigmap.ticksMeasure(0);
      hscroll = new ScrollScale(-2000, -5, xscale, MusEGlobal::song->len(), Qt::Horizontal, this, -offset);
      hscroll->setFocusPolicy(Qt::NoFocus);
      hscroll->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      
      bottomHLayout = new ArrangerHScrollLayout(0, trackInfoButton, 0, hscroll, editor);
      
      box->addLayout(bottomHLayout);
      bottomHLayout->setContentsMargins(0, 0, 0, 0);
      bottomHLayout->setSpacing(0);
      
      vscroll = new QScrollBar(editor);
      vscroll->setMinimum(0);
      vscroll->setMaximum(20*20);
      vscroll->setSingleStep(5);
      vscroll->setPageStep(25); // FIXME: too small steps here for me (flo), better control via window height?
      vscroll->setValue(0);
      vscroll->setOrientation(Qt::Vertical);      

      list->setScroll(vscroll);

      egrid  = new ArrangerCanvasLayout(editor, bottomHLayout);
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
      connect(canvas, SIGNAL(editTrackNameSig()), list, SLOT(editTrackNameSlot()));

      connect(canvas, SIGNAL(muteSelectedTracks()), list, SLOT(muteSelectedTracksSlot()));
      connect(canvas, SIGNAL(soloSelectedTracks()), list, SLOT(soloSelectedTracksSlot()));

      connect(canvas, SIGNAL(horizontalZoom(bool, const QPoint&)), SLOT(horizontalZoom(bool, const QPoint&)));
      connect(canvas, SIGNAL(horizontalZoom(int, const QPoint&)), SLOT(horizontalZoom(int, const QPoint&)));
      connect(lenEntry,           SIGNAL(returnPressed()), SLOT(focusCanvas()));
      connect(lenEntry,           SIGNAL(escapePressed()), SLOT(focusCanvas()));
      connect(globalPitchSpinBox, SIGNAL(returnPressed()), SLOT(focusCanvas()));
      connect(globalPitchSpinBox, SIGNAL(escapePressed()), SLOT(focusCanvas()));
      connect(globalTempoSpinBox, SIGNAL(returnPressed()), SLOT(focusCanvas()));
      connect(globalTempoSpinBox, SIGNAL(escapePressed()), SLOT(focusCanvas()));

      //connect(this,      SIGNAL(redirectWheelEvent(QWheelEvent*)), canvas, SLOT(redirectedWheelEvent(QWheelEvent*)));
      connect(list,      SIGNAL(redirectWheelEvent(QWheelEvent*)), canvas, SLOT(redirectedWheelEvent(QWheelEvent*)));
      
      egrid->addWidget(time, 0, 0, 1, 2);
      egrid->addWidget(MusECore::hLine(editor), 1, 0, 1, 2);

      egrid->addWidget(canvas,  2, 0);
      egrid->addWidget(vscroll, 2, 1);

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

      connect(list, SIGNAL(verticalScrollSetYpos(int)), vscroll, SLOT(setValue(int)));

      connect(canvas, SIGNAL(tracklistChanged()), list, SLOT(tracklistChanged()));
      connect(canvas, SIGNAL(dclickPart(MusECore::Track*)), SIGNAL(editPart(MusECore::Track*)));
      connect(canvas, SIGNAL(startEditor(MusECore::PartList*,int)),   SIGNAL(startEditor(MusECore::PartList*, int)));

      connect(MusEGlobal::song,   SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));
      connect(canvas, SIGNAL(followEvent(int)), hscroll, SLOT(setOffset(int)));

      connect(canvas, SIGNAL(dropSongFile(const QString&)), SIGNAL(dropSongFile(const QString&)));
      connect(canvas, SIGNAL(dropMidiFile(const QString&)), SIGNAL(dropMidiFile(const QString&)));

      connect(canvas, SIGNAL(toolChanged(int)), SIGNAL(toolChanged(int)));
      connect(MusEGlobal::song,   SIGNAL(controllerChanged(MusECore::Track*, int)), SLOT(controllerChanged(MusECore::Track*, int)));

      configChanged();  // set configuration values
      showTrackInfo(showTrackinfoFlag);
      
      setDefaultSplitterSizes();
      
      // Take care of some tabbies!
      setTabOrder(list, canvas);
      }

//---------------------------------------------------------
//   setDefaultSplitterSizes
//---------------------------------------------------------

void Arranger::setDefaultSplitterSizes()
{
  QList<int> vallist;
  vallist.append(trackInfoWidget->minimumSize().width());
//  vallist.append(tlistLayout->sizeHint().width());
  vallist.append(250);
  vallist.append(300);
  split->setSizes(vallist);
}

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
//   setHeaderSizes
//---------------------------------------------------------

void Arranger::setHeaderSizes()
{
    const int fw = 11;

    header->resizeSection(COL_TRACK_IDX, qMax(header->sectionSizeHint(COL_TRACK_IDX) + fw, 30));

    header->resizeSection(COL_INPUT_MONITOR, header->sectionSizeHint(COL_INPUT_MONITOR));
    header->resizeSection(COL_RECORD, header->sectionSizeHint(COL_RECORD));
    header->resizeSection(COL_MUTE, header->sectionSizeHint(COL_MUTE));
    header->resizeSection(COL_SOLO, header->sectionSizeHint(COL_SOLO));
    header->resizeSection(COL_CLASS, header->sectionSizeHint(COL_CLASS));

    header->resizeSection(COL_NAME, qMax(header->sectionSizeHint(COL_NAME) + fw, 100));
    header->resizeSection(COL_OPORT, qMax(header->sectionSizeHint(COL_OPORT) + fw, 60));
    header->resizeSection(COL_OCHANNEL, header->sectionSizeHint(COL_OCHANNEL) + fw);
    header->resizeSection(COL_TIMELOCK, header->sectionSizeHint(COL_TIMELOCK) + fw);
    header->resizeSection(COL_AUTOMATION, qMax(header->sectionSizeHint(COL_AUTOMATION) + fw, 80));
    header->resizeSection(COL_CLEF, qMax(header->sectionSizeHint(COL_CLEF) + fw, 50));

    for (unsigned i = 0; i < custom_columns.size(); i++)
        header->resizeSection(COL_CUSTOM_MIDICTRL_OFFSET + i, qMax(header->sectionSizeHint(COL_CUSTOM_MIDICTRL_OFFSET + i) + fw, 30));
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
      setHeaderSizes();
      _parentWin->updateVisibleTracksButtons();
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
      int newLen = MusEGlobal::sigmap.bar2tick(n, 0, 0);
      MusEGlobal::song->setLen(newLen);
      }
      
//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void Arranger::songChanged(MusECore::SongChangedStruct_t type)
      {
        // We must catch this first and be sure to update the strips.
        if(type & SC_TRACK_REMOVED)
        {
          {
            AudioStrip* w = static_cast<AudioStrip*>(trackInfoWidget->getWidget(1));
            if(w)
            {
              MusECore::Track* t = w->getTrack();
              if(t)
              {
                if(!MusEGlobal::song->trackExists(t))
                {
                  delete w;
                  trackInfoWidget->addWidget(0, 1);
                  selected = 0;
                  switchInfo(0);
                } 
              }   
            } 
          }
          
          {
            MidiStrip* w = static_cast<MidiStrip*>(trackInfoWidget->getWidget(2));
            if(w)
            {
              MusECore::Track* t = w->getTrack();
              if(t)
              {
                if(!MusEGlobal::song->trackExists(t))
                {
                  delete w;
                  trackInfoWidget->addWidget(0, 2);
                  selected = 0;
                  switchInfo(0);
                } 
              }   
            } 
          }
        }
        
        // Try these, may need more/less. 
        if(type & ( SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED | 
           SC_TRACK_MOVED |
           SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED))  
        {
          unsigned endTick = MusEGlobal::song->len();
          int offset  = MusEGlobal::sigmap.ticksMeasure(endTick);
          hscroll->setRange(-offset, endTick + offset);  //DEBUG
          canvas->setOrigin(-offset, 0);
          time->setOrigin(-offset, 0);
    
          int bar, beat;
          unsigned tick;
          MusEGlobal::sigmap.tickValues(endTick, &bar, &beat, &tick);
          if (tick || beat)
                ++bar;
          lenEntry->blockSignals(true);
          lenEntry->setValue(bar);
          lenEntry->blockSignals(false);
        }
        
        if(type & (SC_TRACK_SELECTION | SC_TRACK_INSERTED | SC_TRACK_REMOVED |
          SC_TRACK_MOVED |
          SC_TRACK_MODIFIED | SC_TRACK_RESIZED))
          trackSelectionChanged();
        
        // Keep this light, partsChanged is a heavy move! Try these, may need more. Maybe sig. Requires tempo.
        if(type & (SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED |
                   SC_TRACK_MOVED | SC_TRACK_RESIZED |
                   SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED | 
                   SC_SIG | SC_TEMPO | SC_MASTER)) 
          canvas->updateItems();
        
        if(type & (SC_PART_SELECTION))
        {
          // Prevent race condition: Ignore if the change was ultimately sent by the canvas itself.
          if(type._sender != canvas)
            canvas->updateItemSelections();
        }
        
        if (type & SC_SIG)
              time->redraw();
        if (type & SC_TEMPO)
              setGlobalTempo(MusEGlobal::tempomap.globalTempo());

        // Try these:
        if(type & (SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED |
                   SC_EVENT_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED |
                   SC_CLIP_MODIFIED | SC_MARKER_INSERTED | SC_MARKER_REMOVED | SC_MARKER_MODIFIED))
        canvas->redraw();
        
        // We must marshall song changed instead of connecting to the strip's song changed
        //  otherwise it crashes when loading another song because track is no longer valid
        //  and the strip's songChanged() seems to be called before Arranger songChanged()
        //  gets called and has a chance to stop the crash.
        // Also, calling updateTrackInfo() from here is too heavy, it destroys and recreates
        //  the strips each time no matter what the flags are !
        //updateTrackInfo(type);
        trackInfoSongChange(type);

        // Update the arrangerview's actions.
        // This needs to come after the canvas->selectionChanged() above so that in
        //  selectionChanged(), itemsAreSelected() has the latest citems' selected flags.
        if(type & (SC_TRACK_SELECTION | SC_PART_SELECTION | 
                  SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED | 
                  SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED))
          _parentWin->selectionChanged();
    }

//---------------------------------------------------------
//   trackSelectionChanged
//---------------------------------------------------------

void Arranger::trackSelectionChanged()
      {
      MusECore::Track* track = MusEGlobal::song->selectedTrack();
      if (track == selected)
            return;
      selected = track;
      updateTrackInfo(-1);
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void Arranger::writeStatus(int level, MusECore::Xml& xml)
      {
      xml.tag(level++, "arranger");
      xml.intTag(level, "raster", _raster);
      xml.intTag(level, "info", trackInfoButton->isChecked());
      split->writeStatus(level, xml);

      xml.intTag(level, "xmag", hscroll->mag());
      xml.intTag(level, "xpos", hscroll->pos());
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
                        {
                          // We can only restore the header state with version-compatible data.
                          // If columns were altered, 'alien' loaded data will not fit!
                          if(xml.isVersionEqualToLatest())
                              header_state = QByteArray::fromHex(xml.parse1().toLatin1());
                          else
                            xml.parse1();
                        }
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
      int rast = -1;  
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "raster")
                              rast = xml.parseInt();
                        else if (tag == "info")
                              showTrackinfoFlag = xml.parseInt();
                        else if (tag == split->objectName()) {
                              split->readStatus(xml);
                        }
                        else if (tag == "xmag")
                              hscroll->setMag(xml.parseInt());
                        else if (tag == "xpos") 
                              hscroll->setPos(xml.parseInt());  
                        else if (tag == "ypos")
                              vscroll->setValue(xml.parseInt());
                        else
                              xml.unknown("Arranger");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "arranger") {
                              trackInfoButton->setChecked(showTrackinfoFlag);
                              if(rast != -1)
                                setRasterVal(rast);
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   rasterChanged
//---------------------------------------------------------

void Arranger::rasterChanged(int index)
      {
      _raster = gArrangerRasterTable[index];
      // Set the audio record part snapping.
      MusEGlobal::song->setArrangerRaster(_raster);
      canvas->redraw();
      focusCanvas();
      }

//---------------------------------------------------------
//   setRasterVal
//---------------------------------------------------------

bool Arranger::setRasterVal(int val)
{
  if(_raster == val)
    return true;
  int idx = _rasterCombo->findData(val);
  if(idx == -1)
  {
    fprintf(stderr, "Arranger::setRasterVal raster:%d not found\n", val);
    return false;
  }
  _raster = val;
  _rasterCombo->blockSignals(true);
  _rasterCombo->setCurrentIndex(idx);
  _rasterCombo->blockSignals(false);
  // Set the audio record part snapping.
  MusEGlobal::song->setArrangerRaster(_raster);
  canvas->redraw();
  return true;
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
            case CMD_PASTE_PART_TO_TRACK:
                  ncmd = PartCanvas::CMD_PASTE_PART_TO_TRACK;
                  break;
            case CMD_PASTE_CLONE_PART_TO_TRACK:
                  ncmd = PartCanvas::CMD_PASTE_CLONE_PART_TO_TRACK;
                  break;
            case CMD_PASTE_DIALOG:
                  ncmd = PartCanvas::CMD_PASTE_DIALOG;
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
      MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::SetGlobalTempo, val, 0));
      }

//---------------------------------------------------------
//   setTempo50
//---------------------------------------------------------

void Arranger::setTempo50()
      {
      MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::SetGlobalTempo, 50, 0));
      }

//---------------------------------------------------------
//   setTempo100
//---------------------------------------------------------

void Arranger::setTempo100()
      {
      MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::SetGlobalTempo, 100, 0));
      }

//---------------------------------------------------------
//   setTempo200
//---------------------------------------------------------

void Arranger::setTempo200()
      {
      MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::SetGlobalTempo, 200, 0));
      }

//---------------------------------------------------------
//   setGlobalTempo
//---------------------------------------------------------

void Arranger::setGlobalTempo(int val)
      {
      if(val != globalTempoSpinBox->value())
      {
        globalTempoSpinBox->blockSignals(true);
        globalTempoSpinBox->setValue(val);
        globalTempoSpinBox->blockSignals(false);
      }
      }

//---------------------------------------------------------
//   verticalScrollSetYpos
//---------------------------------------------------------
void Arranger::verticalScrollSetYpos(unsigned ypos)
      {
      vscroll->setValue(ypos);
      }

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void Arranger::clear()
      {
      {
        AudioStrip* w = static_cast<AudioStrip*>(trackInfoWidget->getWidget(1));
        if (w)
              delete w;
        trackInfoWidget->addWidget(0, 1);
      }
      
      {
        MidiStrip* w = static_cast<MidiStrip*>(trackInfoWidget->getWidget(2));
        if (w)
              delete w;
        trackInfoWidget->addWidget(0, 2);
      }
      
      selected = 0;
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
      trackInfoButton->setToolTip(flag ? tr("Hide mixer strip for current track") : tr("Show mixer strip for current track"));
      showTrackinfoFlag = flag;
      trackInfoWidget->setVisible(flag);
      updateTrackInfo(-1);
      }

//---------------------------------------------------------
//   genTrackInfo
//---------------------------------------------------------

void Arranger::genTrackInfo(TrackInfoWidget* trackInfo)
      {
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

      trackInfo->addWidget(noTrackInfo, 0);
      trackInfo->addWidget(0, 1);  // AudioStrip placeholder.
      trackInfo->addWidget(0, 2);  // MidiStrip placeholder.
      }

//---------------------------------------------------------
//   updateTrackInfo
//---------------------------------------------------------

void Arranger::updateTrackInfo(MusECore::SongChangedStruct_t /*flags*/)
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
              switchInfo(2);
            }
      else {
            switchInfo(1);
            }
      }

//---------------------------------------------------------
//   switchInfo
//---------------------------------------------------------

void Arranger::switchInfo(int n)
      {
      if (n == 1) {
          {
            MidiStrip* w = static_cast<MidiStrip*>(trackInfoWidget->getWidget(2));
            if (w)
            {
              //fprintf(stderr, "Arranger::switchInfo audio strip: deleting midi strip\n");
              delete w;
              //w->deleteLater();
              trackInfoWidget->addWidget(0, 2);
            }
          }
          {
              AudioStrip* w = static_cast<AudioStrip*>(trackInfoWidget->getWidget(1));
              if (w == 0 || selected != w->getTrack()) {
                    if (w)
                    {
                          //fprintf(stderr, "Arranger::switchInfo deleting strip\n");
                          delete w;
                          //w->deleteLater();
                    }
                    w = new AudioStrip(trackInfoWidget, static_cast<MusECore::AudioTrack*>(selected));
                    // Broadcast changes to other selected tracks.
                    w->setBroadcastChanges(true);

                    // Set focus yielding to the canvas.
                    if(MusEGlobal::config.smartFocus)
                    {
                      w->setFocusYieldWidget(canvas);
                      //w->setFocusPolicy(Qt::WheelFocus);
                    }

                    // We must marshall song changed instead of connecting to the strip's song changed
                    //  otherwise it crashes when loading another song because track is no longer valid
                    //  and the strip's songChanged() seems to be called before Arranger songChanged()
                    //  gets called and has a chance to stop the crash.
                    //connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), w, SLOT(songChanged(MusECore::SongChangedStruct_t)));
                    
                    connect(MusEGlobal::muse, SIGNAL(configChanged()), w, SLOT(configChanged()));
                    w->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
                    trackInfoWidget->addWidget(w, 1);
                    w->show();
                    //setTabOrder(midiTrackInfo, w);
                    }
          }
        }

      else if (n == 2) {
          {
            AudioStrip* w = static_cast<AudioStrip*>(trackInfoWidget->getWidget(1));
            if (w)
            {
              //fprintf(stderr, "Arranger::switchInfo midi strip: deleting audio strip\n");
              delete w;
              //w->deleteLater();
              trackInfoWidget->addWidget(0, 1);
            }
          }
          {
            MidiStrip* w = static_cast<MidiStrip*>(trackInfoWidget->getWidget(2));
            if (w == 0 || selected != w->getTrack()) {
                  if (w)
                  {
                        //fprintf(stderr, "Arranger::switchInfo deleting strip\n");
                        delete w;
                        //w->deleteLater();
                  }
                  w = new MidiStrip(trackInfoWidget, static_cast<MusECore::MidiTrack*>(selected));
                  // Broadcast changes to other selected tracks.
                  w->setBroadcastChanges(true);
                  // Set focus yielding to the arranger canvas.
                  if(MusEGlobal::config.smartFocus)
                  {
                    w->setFocusYieldWidget(canvas);
                    //w->setFocusPolicy(Qt::WheelFocus);
                  }

                  // No. See above.
                  //connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), w, SLOT(songChanged(MusECore::SongChangedStruct_t)));
                  
                  connect(MusEGlobal::muse, SIGNAL(configChanged()), w, SLOT(configChanged()));
                  w->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
                  trackInfoWidget->addWidget(w, 2);
                  w->show();
                  //setTabOrder(midiTrackInfo, w);
                  }
          }
        }
            
      if (trackInfoWidget->curIdx() == n)
            return;
      trackInfoWidget->raiseWidget(n);
      }

//---------------------------------------------------------
//   trackInfoSongChange
//---------------------------------------------------------

void Arranger::trackInfoSongChange(MusECore::SongChangedStruct_t flags)
{
  if(!selected || !showTrackinfoFlag)
    return;

  // Only update what is showing.
  if(selected->isMidiTrack()) 
  {
    MidiStrip* w = static_cast<MidiStrip*>(trackInfoWidget->getWidget(2));
    if(w)
      w->songChanged(flags);
  }
  else 
  {
    AudioStrip* w = static_cast<AudioStrip*>(trackInfoWidget->getWidget(1));
    if(w)
      w->songChanged(flags);
  }
}

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
        horizontalZoom(true, QCursor::pos());
        return;
        }
  else if (key == shortcuts[SHRT_ZOOM_OUT].key) {
        horizontalZoom(false, QCursor::pos());
        return;
        }

  QWidget::keyPressEvent(event);
}

void Arranger::horizontalZoom(bool zoom_in, const QPoint& glob_pos)
{
  int mag = hscroll->mag();
  int zoomlvl = ScrollScale::getQuickZoomLevel(mag);
  if(zoom_in)
  {
    if (zoomlvl < MusEGui::ScrollScale::zoomLevels-1)
        zoomlvl++;
  }
  else
  {
    if (zoomlvl > 1)
        zoomlvl--;
  }
  int newmag = ScrollScale::convertQuickZoomLevelToMag(zoomlvl);

  QPoint cp = canvas->mapFromGlobal(glob_pos);
  QPoint sp = editor->mapFromGlobal(glob_pos);
  if(cp.x() >= 0 && cp.x() < canvas->width() && sp.y() >= 0 && sp.y() < editor->height())
    hscroll->setMag(newmag, cp.x());
}

void Arranger::horizontalZoom(int mag, const QPoint& glob_pos)
{
  QPoint cp = canvas->mapFromGlobal(glob_pos);
  QPoint sp = editor->mapFromGlobal(glob_pos);
  if(cp.x() >= 0 && cp.x() < canvas->width() && sp.y() >= 0 && sp.y() < editor->height())
    hscroll->setMag(hscroll->mag() + mag, cp.x());
}


} // namespace MusEGui
