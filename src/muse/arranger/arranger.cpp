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

//#include "config.h"

#include <stdio.h>
#include <limits.h>
#include "muse_math.h"

//#include <QComboBox>
#include <QGridLayout>
//#include <QKeyEvent>
#include <QLabel>
#include <QList>
//#include <QMainWindow>
#include <QScrollBar>
#include <QToolBar>
//#include <QVBoxLayout>
#include <QWheelEvent>
#include <QPainter>
#include <QCursor>
#include <QPoint>
//#include <QRect>
#include <QSettings>

#include "arranger.h"
#include "song.h"
#include "app.h"
#include "midiport.h"
//#include "mididev.h"
#include "utils.h"
#include "globals.h"
#include "globaldefs.h"
#include "icons.h"
//#include "utils.h"
#include "widget_stack.h"
#include "audio.h"
//#include "event.h"
#include "midiseq.h"
//#include "midictrl.h"
//#include "mpevent.h"
#include "gconfig.h"
#include "mixer/astrip.h"
#include "mixer/mstrip.h"
#include "shortcuts.h"
//#include "ttoolbutton.h"

// Forwards from header:
//#include <QKeyEvent>
//#include <QPoint>
//#include <QComboBox>
//#include <QScrollBar>
//#include <QVBoxLayout>
//#include <QHBoxLayout>
#include <QScrollArea>
#include <QToolButton>
#include "track.h"
//#include "part.h"
#include "xml.h"
#include "rasterizer.h"
//#include "lcombo.h"
#include "mtscale.h"
#include "arrangerview.h"
//#include "astrip.h"
#include "header.h"
#include "poslabel.h"
#include "scrollscale.h"
#include "spinbox.h"
#include "splitter.h"
#include "trackinfo_layout.h"
#include "tlist.h"
#include "raster_widgets.h"
#include "pcanvas.h"

namespace MusEGui {

std::vector<Arranger::custom_col_t> Arranger::custom_columns;
QByteArray Arranger::header_state;

void Arranger::writeCustomColumns(int level, MusECore::Xml& xml)
{
  xml.tag(level++, "custom_columns");
  
  for (unsigned i = 0; i < custom_columns.size(); i++)
  {
    xml.tag(level++, "column");
    xml.strTag(level, "name", custom_columns[i].name);
    xml.intTag(level, "ctrl", custom_columns[i].ctrl);
    xml.intTag(level, "affected_pos", custom_columns[i].affected_pos);
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
    header->setToolTip(TList::COL_TRACK_IDX,  tr("Track index"));
    header->setToolTip(TList::COL_INPUT_MONITOR, tr("Input monitor"));
    header->setToolTip(TList::COL_RECORD,     tr("Recording"));
    header->setToolTip(TList::COL_MUTE,       tr("Mute/Off indicator"));
    header->setToolTip(TList::COL_SOLO,       tr("Solo indicator"));
    header->setToolTip(TList::COL_CLASS,      tr("Track type"));
    header->setToolTip(TList::COL_NAME,       tr("Track name"));
    header->setToolTip(TList::COL_OCHANNEL,   tr("Midi output channel number or number of audio channels"));
    header->setToolTip(TList::COL_OPORT,      tr("Midi output port or synth GUI"));
//    header->setToolTip(TList::COL_TIMELOCK,   tr("Time lock"));
    header->setToolTip(TList::COL_AUTOMATION, tr("Automation parameter selection"));
    header->setToolTip(TList::COL_CLEF,       tr("Notation clef"));
}



//---------------------------------------------------------
//   Arranger::setHeaderWhatsThis
//---------------------------------------------------------

void Arranger::setHeaderWhatsThis()
{
    header->setWhatsThis(TList::COL_TRACK_IDX, tr("Track index"));
    header->setWhatsThis(TList::COL_INPUT_MONITOR, tr("Enable input monitor. Click to toggle.\nPasses input through to output for monitoring.\n"
                                                      "See also Settings: Automatically Monitor On Record Arm."));
    header->setWhatsThis(TList::COL_RECORD,   tr("Enable recording. Click to toggle.\n"
                                                 "See also Settings: Automatically Monitor On Record Arm."));
    header->setWhatsThis(TList::COL_MUTE,     tr("Mute indicator. Click to toggle.\nRight-click to toggle track on/off.\nMute is designed for rapid, repeated action.\nOn/Off is not!"));
    header->setWhatsThis(TList::COL_SOLO,     tr("Solo indicator. Click to toggle.\nConnected tracks are also 'phantom' soloed."));
    header->setWhatsThis(TList::COL_CLASS,    tr("Track type. Right-click to change\n midi and drum track types."));
    header->setWhatsThis(TList::COL_NAME,     tr("Track name. Double-click to edit.\nRight-click for more options."));
    header->setWhatsThis(TList::COL_OCHANNEL, tr("Midi/Drum track: Output channel number.\nAudio track: Channels.\nMid/right-click to change."));
    header->setWhatsThis(TList::COL_OPORT,    tr("Midi/Drum track: Output port.\nSynth track: Right-click to show GUI."));
//    header->setWhatsThis(TList::COL_TIMELOCK, tr("Time lock"));
    header->setWhatsThis(TList::COL_CLEF,     tr("Notation clef. Select this tracks notation clef."));
}

//---------------------------------------------------------
//   Arranger::setHeaderStatusTips
//---------------------------------------------------------

void Arranger::setHeaderStatusTips()
{
    header->setStatusTip(TList::COL_TRACK_IDX, tr("Track index: Double-click to select all tracks (+SHIFT to select all tracks of the same type)."));
    header->setStatusTip(TList::COL_INPUT_MONITOR, tr("Input monitor: Left click to toggle current/selected, right click for all tracks of same type."));
    header->setStatusTip(TList::COL_RECORD,   tr("Recording: LMB to toggle current/selected, RMB for all tracks of same type. Audio output: LMB to downmix to a file."));
    header->setStatusTip(TList::COL_MUTE,     tr("Mute indicator: Left click to mute, right click to switch on/off (+CTRL for all tracks except audio outputs)."));
    header->setStatusTip(TList::COL_SOLO,     tr("Solo indicator: Click to solo (+CTRL for all tracks except audio outputs). Connected tracks are 'phantom' soloed."));
    header->setStatusTip(TList::COL_CLASS,    tr("Track type (RMB for context menu): MIDI: Switch track types. Synth: Open GUI. Audio output: Downmix."));
    header->setStatusTip(TList::COL_NAME,     tr("Track name: Double-click to edit. RMB for context menu."));
    header->setStatusTip(TList::COL_OCHANNEL, tr("Midi/Drum: Output channel number. Audio: Number of channels. MMB / (CTRL/SHIFT+)RMB / Double-click to change."));
    header->setStatusTip(TList::COL_OPORT,    tr("Midi/Drum: RMB to set the output port (+CTRL for all tracks of same type). Synth: RMB to show synth GUI."));
//    header->setStatusTip(TList::COL_TIMELOCK, tr("Time lock"));
    header->setStatusTip(TList::COL_AUTOMATION, tr("Automation: RMB to select parameters."));
    header->setStatusTip(TList::COL_CLEF,     tr("Notation clef: RMB to select this track's notation clef."));
}

//---------------------------------------------------------
//   Arranger
//    is the central widget in app
//---------------------------------------------------------

Arranger::Arranger(ArrangerView* parent, const char* name)
   : QWidget(parent)
      {
      setObjectName(name);

      _canvasXOrigin = DefaultCanvasXOrigin;
      //_minXMag = -2000;
      _minXMag = -500;
      _maxXMag = -5;
      QList<Rasterizer::Column> rast_cols;
      rast_cols <<
        Rasterizer::TripletColumn <<
        Rasterizer::NormalColumn <<
        Rasterizer::DottedColumn;
      _rasterizerModel = new RasterizerModel(
      MusEGlobal::globalRasterizer, this, 7, rast_cols);
      _rasterizerModel->setDisplayFormat(RasterizerModel::FractionFormat);

      _raster = _rasterizerModel->pickRaster(0, RasterizerModel::GotoBar);
      selected = nullptr;
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
      toolbar->addWidget(label);
      cursorPos = new PosLabel(nullptr, "PosLabel");
      cursorPos->setToolTip(tr("Cursor position"));
      cursorPos->setEnabled(false);
      toolbar->addWidget(cursorPos);

      gridOnButton = new QToolButton();
      gridOnButton->setIcon(*gridOnSVGIcon);
      gridOnButton->setFocusPolicy(Qt::NoFocus);
      gridOnButton->setCheckable(true);
      gridOnButton->setToolTip(tr("Show grid"));
      gridOnButton->setWhatsThis(tr("Show grid"));
      toolbar->addWidget(gridOnButton);
      connect(gridOnButton, &QToolButton::toggled, [this](bool v) { gridOnChanged(v); } );

      _rasterCombo = new RasterLabelCombo(RasterLabelCombo::TableView, _rasterizerModel, this, "RasterLabelCombo");
      _rasterCombo->setFocusPolicy(Qt::TabFocus);
      toolbar->addWidget(_rasterCombo);

      // Song len
      label = new QLabel(tr("Bars"));
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
      QSizePolicy tipolicy = QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
      tipolicy.setHorizontalStretch(0);
      tipolicy.setVerticalStretch(100);
      trackInfoWidget->setSizePolicy(tipolicy);
      
      tracklistScroll = new QScrollArea(split);
      split->setStretchFactor(split->indexOf(tracklistScroll), 0);
      QSizePolicy tpolicy = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
      tpolicy.setHorizontalStretch(0);
      tpolicy.setVerticalStretch(100);
      tracklistScroll->setSizePolicy(tpolicy);
      tracklistScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
      tracklistScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      tracklistScroll->setWidgetResizable(true);

      editor = new QWidget(split);
      split->setStretchFactor(split->indexOf(editor), 1);
      QSizePolicy epolicy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      epolicy.setHorizontalStretch(255);
      epolicy.setVerticalStretch(100);
      editor->setSizePolicy(epolicy);

      //---------------------------------------------------
      //    Track Info
      //---------------------------------------------------

      genTrackInfo(trackInfoWidget);

      tracklist = new QWidget(tracklistScroll);
      initTracklistHeader();
      list = new TList(header, tracklist, "TrackList");

      tlistLayout = new QVBoxLayout(tracklist);
      tlistLayout->setContentsMargins(0, 0, 0, 0);
      tlistLayout->setSpacing(0);
      tlistLayout->addWidget(header);
      tlistLayout->addWidget(list);

      tracklist->setMinimumWidth(header->length());
      tracklistScroll->setWidget(tracklist);

      connect(header, SIGNAL(sectionResized(int,int,int)), this, SLOT(updateTracklist()));
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
      hscroll = new ScrollScale(
        (_minXMag * MusEGlobal::config.division) / 384,
        _maxXMag,
        xscale,
        MusEGlobal::song->len() + offset,
        Qt::Horizontal,
        this,
        _canvasXOrigin);

      hscroll->setFocusPolicy(Qt::NoFocus);
      hscroll->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      
      bottomHLayout = new QHBoxLayout();
      bottomHLayout->addWidget(hscroll);
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

      egrid  = new QGridLayout(editor);
      egrid->setColumnStretch(0, 50);
      egrid->setRowStretch(2, 50);
      egrid->setContentsMargins(0, 0, 0, 0);  
      egrid->setSpacing(0);  

      time = new MTScale(_raster, editor, xscale);
      time->setOrigin(_canvasXOrigin, 0);
      canvas = new PartCanvas(&_raster, editor, xscale, yscale);
      canvas->setBg(MusEGlobal::config.partCanvasBg);
      canvas->setCanvasTools(arrangerTools);
      canvas->setOrigin(_canvasXOrigin, 0);
      canvas->setFocus();

      list->setFocusProxy(canvas); // Make it easy for track list popup line editor to give focus back to canvas.

      setRasterVal(_raster);

      connect(canvas, SIGNAL(setUsedTool(int)), this, SIGNAL(setUsedTool(int)));
      connect(canvas, SIGNAL(trackChanged(MusECore::Track*)), list, SLOT(selectTrack(MusECore::Track*)));
      connect(list, SIGNAL(keyPressExt(QKeyEvent*)), canvas, SLOT(redirKeypress(QKeyEvent*)));
      connect(canvas, SIGNAL(selectTrackAbove()), list, SLOT(selectTrackAbove()));
      connect(canvas, SIGNAL(selectTrackBelow()), list, SLOT(selectTrackBelow()));
      connect(canvas, SIGNAL(editTrackNameSig()), list, SLOT(editTrackNameSlot()));

      connect(canvas, SIGNAL(muteSelectedTracks()), list, SLOT(muteSelectedTracksSlot()));
      connect(canvas, SIGNAL(soloSelectedTracks()), list, SLOT(soloSelectedTracksSlot()));

      connect(canvas, SIGNAL(volumeSelectedTracks(int)), list, SLOT(volumeSelectedTracksSlot(int)));
      connect(canvas, SIGNAL(panSelectedTracks(int)), list, SLOT(panSelectedTracksSlot(int)));

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
      egrid->addLayout(bottomHLayout, 3, 0);

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

      connect(_rasterCombo, &RasterLabelCombo::rasterChanged, [this](int raster) { rasterChanged(raster); } );

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
    QSettings s;
    if (split->restoreState(s.value("Arranger/splitState").toByteArray()))
        return;

    QList<int> vallist;
    vallist.append(trackInfoWidget->sizeHint().width());
    list->resize(250, 100);
    vallist.append(tlistLayout->sizeHint().width());
    vallist.append(1);
    split->setSizes(vallist);
}

void Arranger::storeSplitterSizes() {
    QSettings s;
    s.setValue("Arranger/splitState", split->saveState());
}

void Arranger::initTracklistHeader()
{
    header = new Header(tracklist, "TrackListHeader");
    header->setFixedHeight(31);

    header->setColumnLabel("#", TList::COL_TRACK_IDX);
    header->setColumnIcon(*monitorOnSVGIcon, TList::COL_INPUT_MONITOR);
    header->setColumnIcon(*recArmOnSVGIcon, TList::COL_RECORD);
    header->setColumnIcon(*muteOnSVGIcon, TList::COL_MUTE);
    header->setColumnIcon(*soloOnAloneSVGIcon, TList::COL_SOLO);
    header->setColumnIcon(*tracktypeSVGIcon, TList::COL_CLASS);
    header->setColumnLabel(tr("Track"), TList::COL_NAME);
    header->setColumnLabel(tr("Port"), TList::COL_OPORT);
    //: Channel
    header->setColumnLabel(tr("Ch"), TList::COL_OCHANNEL);
    //: Time lock
//    header->setColumnLabel(tr("T"), TList::COL_TIMELOCK);
    header->setColumnLabel(tr("Automation"), TList::COL_AUTOMATION);
    header->setColumnLabel(tr("Clef"), TList::COL_CLEF);
    for (unsigned i = 0; i < custom_columns.size(); i++)
        header->setColumnLabel(custom_columns[i].name, TList::COL_CUSTOM_MIDICTRL_OFFSET + i);

    header->setSectionResizeMode(TList::COL_TRACK_IDX, QHeaderView::Interactive);
    header->setSectionResizeMode(TList::COL_INPUT_MONITOR, QHeaderView::Fixed);
    header->setSectionResizeMode(TList::COL_RECORD, QHeaderView::Fixed);
    header->setSectionResizeMode(TList::COL_MUTE, QHeaderView::Fixed);
    header->setSectionResizeMode(TList::COL_SOLO, QHeaderView::Fixed);
    header->setSectionResizeMode(TList::COL_CLASS, QHeaderView::Fixed);
    header->setSectionResizeMode(TList::COL_NAME, QHeaderView::Interactive);
    header->setSectionResizeMode(TList::COL_OPORT, QHeaderView::Interactive);
    header->setSectionResizeMode(TList::COL_OCHANNEL, QHeaderView::Fixed);
//    header->setSectionResizeMode(TList::COL_TIMELOCK, QHeaderView::Fixed);
    header->setSectionResizeMode(TList::COL_AUTOMATION, QHeaderView::Interactive);
    header->setSectionResizeMode(TList::COL_CLEF, QHeaderView::Interactive);
    for (unsigned i = 0; i < custom_columns.size(); i++)
        header->setSectionResizeMode(TList::COL_CUSTOM_MIDICTRL_OFFSET+i, QHeaderView::Interactive);

    // 04/18/17 Time lock remains unused. Disabled until a use is found.
    // Plans were to use it (or not) when time stretching / pitch shifting work is done.
//    header->setSectionHidden(TList::COL_TIMELOCK, true);

    setHeaderToolTips();
    setHeaderWhatsThis();
    setHeaderStatusTips(); // does not work with Qt 5.9!
    header->setSectionsMovable (true);
    header->restoreState(header_state);
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

    header->resizeSection(TList::COL_TRACK_IDX, qMax(header->sectionSizeHint(TList::COL_TRACK_IDX) + fw, 30));

    header->resizeSection(TList::COL_INPUT_MONITOR, header->sectionSizeHint(TList::COL_INPUT_MONITOR));
    header->resizeSection(TList::COL_RECORD, header->sectionSizeHint(TList::COL_RECORD));
    header->resizeSection(TList::COL_MUTE, header->sectionSizeHint(TList::COL_MUTE));
    header->resizeSection(TList::COL_SOLO, header->sectionSizeHint(TList::COL_SOLO));
    header->resizeSection(TList::COL_CLASS, header->sectionSizeHint(TList::COL_CLASS));

    header->resizeSection(TList::COL_NAME, qMax(header->sectionSizeHint(TList::COL_NAME) + fw, 100));
    header->resizeSection(TList::COL_OPORT, qMax(header->sectionSizeHint(TList::COL_OPORT) + fw, 60));
    header->resizeSection(TList::COL_OCHANNEL, header->sectionSizeHint(TList::COL_OCHANNEL) + fw);
//    header->resizeSection(TList::COL_TIMELOCK, header->sectionSizeHint(TList::COL_TIMELOCK) + fw);
    header->resizeSection(TList::COL_AUTOMATION, qMax(header->sectionSizeHint(TList::COL_AUTOMATION) + fw, 80));
    header->resizeSection(TList::COL_CLEF, qMax(header->sectionSizeHint(TList::COL_CLEF) + fw, 50));

    for (unsigned i = 0; i < custom_columns.size(); i++)
        header->resizeSection(TList::COL_CUSTOM_MIDICTRL_OFFSET + i, qMax(header->sectionSizeHint(TList::COL_CUSTOM_MIDICTRL_OFFSET + i) + fw, 30));
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

      gridOnButton->blockSignals(true);
      gridOnButton->setChecked(MusEGlobal::config.canvasShowGrid);
      gridOnButton->blockSignals(false);

      canvas->redraw();
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
                  trackInfoWidget->addWidget(nullptr, 1);
                  selected = nullptr;
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
                  trackInfoWidget->addWidget(nullptr, 2);
                  selected = nullptr;
                  switchInfo(0);
                } 
              }   
            } 
          }
        }
        
        // Try these, may need more/less. 
        if(type & ( SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED | SC_TRACK_MOVED |
           SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED |
           SC_SIG))
        {
          unsigned endTick = MusEGlobal::song->len();
          int offset  = MusEGlobal::sigmap.ticksMeasure(endTick);
          hscroll->setRange(_canvasXOrigin, endTick + offset);
          canvas->setOrigin(_canvasXOrigin, 0);
          time->setOrigin(_canvasXOrigin, 0);
    
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

        if (type & SC_DIVISION_CHANGED)
        {
          // The division has changed. The raster table and raster model will have been
          //  cleared and re-filled, so any views on the model will no longer have a
          //  current item and our current raster value will be invalid. They WILL NOT
          //  emit an activated signal. So we must manually select a current item and
          //  raster value here. We could do something fancy to try to keep the current
          //  index - for example stay on quarter note - by taking the ratio of the new
          //  division to old division and apply that to the old raster value and try
          //  to select that index, but the division has already changed.
          // So instead, simply try to select the current raster value. The index in the box may change.
          // Be sure to use what it chooses.
          setRasterVal(_raster);

          // Now set a reasonable zoom (mag) range.
          setupHZoomRange();
        }
        
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
      xml.intTag(level, "info", showTrackinfoFlag);
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

void Arranger::rasterChanged(int raster)
      {
      _raster = raster;
      time->setRaster(_raster);
      canvas->redraw();
      focusCanvas();
      }

//---------------------------------------------------------
//   rasterVal
//---------------------------------------------------------

int Arranger::rasterVal() const
{
  return _raster;
}

//---------------------------------------------------------
//   currentPartColorIndex
//---------------------------------------------------------

int Arranger::currentPartColorIndex() const
{
  if(canvas)
    return canvas->currentPartColorIndex();
  return 0;
}

//---------------------------------------------------------
//   setRasterVal
//---------------------------------------------------------

bool Arranger::setRasterVal(int val)
{
  const RasterizerModel* rast_mdl = _rasterCombo->rasterizerModel();
  _raster = rast_mdl->checkRaster(val);
  time->setRaster(_raster);
  const QModelIndex mdl_idx = rast_mdl->modelIndexOfRaster(_raster);
  if(mdl_idx.isValid())
    _rasterCombo->setCurrentModelIndex(mdl_idx);
  else
    fprintf(stderr, "Arranger::changeRaster: _raster %d not found in box!\n", _raster);
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
//   gridOnChanged
//---------------------------------------------------------

void Arranger::gridOnChanged(bool v)
{
  MusEGlobal::config.canvasShowGrid = v;
  // We want the simple version, don't set the style or stylesheet yet.
  MusEGlobal::muse->changeConfig(true);
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
//   toggleTrackInfo
//---------------------------------------------------------

void Arranger::toggleTrackInfo()
{
    showTrackInfo(showTrackinfoFlag ^ true);
}

//---------------------------------------------------------
//   showTrackInfo
//---------------------------------------------------------

void Arranger::showTrackInfo(bool flag)
{
    showTrackinfoFlag = flag;
    trackInfoWidget->setVisible(flag);
    updateTrackInfo(-1);
}

//---------------------------------------------------------
//   genTrackInfo
//---------------------------------------------------------

void Arranger::genTrackInfo(TrackInfoWidget* trackInfo)
      {
      trackInfo->addWidget(nullptr, 1);  // AudioStrip placeholder.
      trackInfo->addWidget(nullptr, 2);  // MidiStrip placeholder.
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
      if (selected == nullptr) {
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
              trackInfoWidget->addWidget(nullptr, 2);
            }
          }
          {
              AudioStrip* w = static_cast<AudioStrip*>(trackInfoWidget->getWidget(1));
              if (w == nullptr || selected != w->getTrack()) {
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
              trackInfoWidget->addWidget(nullptr, 1);
            }
          }
          {
            MidiStrip* w = static_cast<MidiStrip*>(trackInfoWidget->getWidget(2));
            if (w == nullptr || selected != w->getTrack()) {
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

  RasterizerModel::RasterPick rast_pick = RasterizerModel::NoPick;
  const int cur_rast = rasterVal();

  if (key == shortcuts[SHRT_ZOOM_IN].key) {
        horizontalZoom(true, QCursor::pos());
        return;
        }
  else if (key == shortcuts[SHRT_ZOOM_OUT].key) {
        horizontalZoom(false, QCursor::pos());
        return;
        }
  else if (key == shortcuts[SHRT_HIDE_MIXER_STRIP].key) {
      showTrackInfo(!showTrackinfoFlag);
      return;
  }
  // QUANTIZE shortcuts from midi editors is reused for SNAP in Arranger
  //    `does not work exactly the same but close enough I think.
  else if (key == shortcuts[SHRT_SET_QUANT_BAR].key) {
      rast_pick = RasterizerModel::GotoBar;
  }
  else if (key == shortcuts[SHRT_SET_QUANT_OFF].key) {
      rast_pick = RasterizerModel::GotoOff;
  }
  else if (key == shortcuts[SHRT_SET_QUANT_1].key) {
      rast_pick = RasterizerModel::Goto1;
  }
  else if (key == shortcuts[SHRT_SET_QUANT_2].key) {
      rast_pick = RasterizerModel::Goto2;
  }
  else if (key == shortcuts[SHRT_SET_QUANT_3].key) {
      rast_pick = RasterizerModel::Goto4;
  }
  else if (key == shortcuts[SHRT_SET_QUANT_4].key) {
      rast_pick = RasterizerModel::Goto8;
  }
  else if (key == shortcuts[SHRT_SET_QUANT_5].key) {
      rast_pick = RasterizerModel::Goto16;
  }
  else if (key == shortcuts[SHRT_SET_QUANT_6].key) {
      // this value is not actually defined, adding for completeness but commented out.
      return;
      //rast_pick = RasterizerModel::Goto32;
  }
  else if (key == shortcuts[SHRT_SET_QUANT_7].key) {
      // this value is not actually defined, adding for completeness but commented out.
      return;
      //rast_pick = RasterizerModel::Goto64;
  }

  if(rast_pick != RasterizerModel::NoPick)
  {
    const int new_rast = _rasterizerModel->pickRaster(cur_rast, rast_pick);
    if(new_rast != cur_rast)
      setRasterVal(new_rast);
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

void Arranger::updateHeaderCustomColumns()
{
    for (int i = TList::COL_CUSTOM_MIDICTRL_OFFSET; i < header->count(); i++)
        header->removeColumn(i);

    header->headerDataChanged(Qt::Horizontal, TList::COL_CUSTOM_MIDICTRL_OFFSET, header->count());

    for (unsigned i = 0; i < custom_columns.size(); i++) {
        header->setColumnLabel(custom_columns[i].name, TList::COL_CUSTOM_MIDICTRL_OFFSET + i);
        header->showSection(TList::COL_CUSTOM_MIDICTRL_OFFSET + i);
    }

    setHeaderSizes();
    updateTracklist();
}

void Arranger::updateTracklist()
{
    tracklist->setMinimumWidth(header->length());
    list->redraw();
}

bool Arranger::isSingleSelection() const { return canvas->isSingleSelection(); }
int Arranger::selectionSize() const { return canvas->selectionSize(); }
bool Arranger::itemsAreSelected() const { return canvas->itemsAreSelected(); }
void Arranger::songIsClearing() const { canvas->songIsClearing(); }

//---------------------------------------------------------
//   setupHZoomRange
//---------------------------------------------------------

void Arranger::setupHZoomRange()
{
  const int min = (_minXMag * MusEGlobal::config.division) / 384;
  hscroll->setScaleRange(min, _maxXMag);
}

} // namespace MusEGui
