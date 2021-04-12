//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: waveedit.cpp,v 1.5.2.12 2009/04/06 01:24:54 terminator356 Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include <limits.h>

#include <QLayout>
#include <QSizeGrip>
#include <QScrollBar>
#include <QLabel>
#include <QMenuBar>
#include <QSettings>
#include <QCursor>
#include <QRect>

#include "app.h"
#include "waveedit.h"
#include "mtscale.h"
#include "wavecanvas.h"
#include "globals.h"
#include "globaldefs.h"
#include "audio.h"
#include "utils.h"
#include "song.h"
#include "gconfig.h"
#include "icons.h"
#include "shortcuts.h"
#include "cmd.h"
#include "operations.h"
#include "trackinfo_layout.h"

// Forwards from header:
#include <QMenu>
#include <QWidget>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QAction>
#include <QSlider>
#include <QToolButton>
#include <QPoint>
#include <QToolBar>
#include <QModelIndex>
#include "part.h"
#include "xml.h"
#include "splitter.h"
#include "poslabel.h"
#include "scrollscale.h"
#include "cobject.h"
#include "tools.h"
#include "raster_widgets.h"

// For debugging output: Uncomment the fprintf section.
#define ERROR_WAVEEDIT(dev, format, args...)  fprintf(dev, format, ##args)
#define DEBUG_WAVEEDIT(dev, format, args...) // fprintf(dev, format, ##args)

namespace MusECore {
extern QColor readColor(MusECore::Xml& xml);
}

namespace MusEGui {

static int waveEditTools = PointerTool | PencilTool | RubberTool |
//                           CutTool | // unused
                           RangeTool | PanTool | ZoomTool |
                           StretchTool | SamplerateTool;

int WaveEdit::_rasterInit = 96;
int WaveEdit::_trackInfoWidthInit = 50;
int WaveEdit::_canvasWidthInit = 300;
int WaveEdit::colorModeInit = 0;
  
//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void WaveEdit::closeEvent(QCloseEvent* e)
      {
      _isDeleting = true;  // Set flag so certain signals like songChanged, which may cause crash during delete, can be ignored.

      storeSettings();

      emit isDeleting(static_cast<TopWin*>(this));
      e->accept();
      }

void WaveEdit::storeSettings() {

    QSettings settings;
    //settings.setValue("Waveedit/geometry", saveGeometry());
    settings.setValue("Waveedit/windowState", saveState());

    //Store values of the horizontal splitter
    QList<int> sizes = hsplitter->sizes();
    QList<int>::iterator it = sizes.begin();
    _trackInfoWidthInit = *it; //There are only 2 values stored in the sizelist, size of trackinfo widget and canvas widget
    it++;
    _canvasWidthInit = *it;
}

//---------------------------------------------------------
//   WaveEdit
//---------------------------------------------------------

WaveEdit::WaveEdit(MusECore::PartList* pl, QWidget* parent, const char* name)
   : MidiEditor(TopWin::WAVE, _rasterInit, pl, parent, name)
      {
      setFocusPolicy(Qt::NoFocus);
      colorMode      = colorModeInit;

      // Request to set the raster, but be sure to use the one it chooses,
      //  which may be different than the one requested.
      _rasterInit = _rasterizerModel->checkRaster(_rasterInit);
      _raster = _rasterInit;
      _canvasXOrigin = DefaultCanvasXOrigin;
      //_minXMag = -32768;
      _minXMag = -16384;
      _maxXMag = -1;

      QAction* act;
      
      QMenu* menuEdit = menuBar()->addMenu(tr("&Edit"));
      
      cutAction = menuEdit->addAction(*cutSVGIcon, tr("C&ut"));
      connect(cutAction, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_EDIT_CUT); } );

      copyAction = menuEdit->addAction(*copySVGIcon, tr("&Copy"));
      connect(copyAction, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_EDIT_COPY); } );

      pasteAction = menuEdit->addAction(*pasteSVGIcon, tr("&Paste"));
      connect(pasteAction, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_EDIT_PASTE); } );

      menuEdit->addSeparator();

      copyPartRegionAction = menuEdit->addAction(tr("Create Part &from Range"));
      connect(copyPartRegionAction, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_CREATE_PART_REGION); } );

      act = menuEdit->addAction(tr("Edit in E&xternal Editor..."));
      connect(act, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_EDIT_EXTERNAL); } );

      menuEdit->addSeparator();

//      select = menuEdit->addMenu(QIcon(*selectIcon), tr("Select"));

      selectAllAction = menuEdit->addAction(*selectAllSVGIcon, tr("Select &All"));
      connect(selectAllAction, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_SELECT_ALL); } );

      selectNoneAction = menuEdit->addAction(*deselectAllSVGIcon, tr("&Deselect All"));
      connect(selectNoneAction, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_SELECT_NONE); } );

      menuEdit->addSeparator();

      selectRangeToSelectionAction = menuEdit->addAction(*rangeToSelectionSVGIcon, tr("Set &Range to Selection"));
      connect(selectRangeToSelectionAction, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_RANGE_TO_SELECTION); } );

      if (parts()->size() > 1) {
          menuEdit->addSeparator();
          selectNextPartAction = menuEdit->addAction(*nextPartSVGIcon, tr("&Next Part"));
          selectPrevPartAction = menuEdit->addAction(*lastPartSVGIcon, tr("&Previous Part"));
          connect(selectNextPartAction, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_SELECT_NEXT_PART); } );
          connect(selectPrevPartAction, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_SELECT_PREV_PART); } );
      } else {
          selectPrevPartAction = nullptr;
          selectNextPartAction = nullptr;
      }

      menuFunctions = menuBar()->addMenu(tr("Func&tions"));

      menuGain = menuFunctions->addMenu(tr("&Gain"));
      
      act = menuGain->addAction("200%");
      connect(act, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_GAIN_200); } );
      
      act = menuGain->addAction("150%");
      connect(act, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_GAIN_150); } );
      
      act = menuGain->addAction("75%");
      connect(act, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_GAIN_75); } );
      
      act = menuGain->addAction("50%");
      connect(act, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_GAIN_50); } );
      
      act = menuGain->addAction("25%");
      connect(act, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_GAIN_25); } );
      
      act = menuGain->addAction(tr("Other"));
      connect(act, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_GAIN_FREE); } );
      
      menuFunctions->addSeparator();

// REMOVE Tim. Also remove CMD_ADJUST_WAVE_OFFSET and so on...      
//       adjustWaveOffsetAction = menuEdit->addAction(tr("Adjust wave offset..."));
//       mapper->setMapping(adjustWaveOffsetAction, WaveCanvas::CMD_ADJUST_WAVE_OFFSET);
//       connect(adjustWaveOffsetAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuFunctions->addAction(tr("Mute Selection"));
      connect(act, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_MUTE); } );
      
      act = menuFunctions->addAction(tr("Normalize Selection"));
      connect(act, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_NORMALIZE); } );
      
      act = menuFunctions->addAction(tr("Fade-in Selection"));
      connect(act, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_FADE_IN); } );
      
      act = menuFunctions->addAction(tr("Fade-out Selection"));
      connect(act, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_FADE_OUT); } );
      
      act = menuFunctions->addAction(tr("Reverse Selection"));
      connect(act, &QAction::triggered, [this]() { cmd(WaveCanvas::CMD_REVERSE); } );
      

      QMenu* settingsMenu = menuBar()->addMenu(tr("&Display"));
      settingsMenu->menuAction()->setStatusTip(tr("Display menu: Display options specific to current editor."));

      settingsMenu->addAction(subwinAction);
//      settingsMenu->addAction(shareAction);
      settingsMenu->addAction(fullscreenAction);

      settingsMenu->addSeparator();

      eventColor = settingsMenu->addMenu(tr("&Event Color"));
      eventColor->setObjectName("CheckmarkOnly");
      
      QActionGroup* actgrp = new QActionGroup(this);
      actgrp->setExclusive(true);
      
      evColorNormalAction = actgrp->addAction(tr("&Part Colors"));
      evColorNormalAction->setCheckable(true);
      connect(evColorNormalAction, &QAction::triggered, [this]() { eventColorModeChanged(0); } );
      
      evColorPartsAction = actgrp->addAction(tr("&Gray"));
      evColorPartsAction->setCheckable(true);
      connect(evColorPartsAction, &QAction::triggered, [this]() { eventColorModeChanged(1); } );
      
      eventColor->addActions(actgrp->actions());
      
      connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));


      //--------------------------------------------------
      //    ToolBar:   Solo  Cursor1 Cursor2

      // NOTICE: Please ensure that any tool bar object names here match the names assigned 
      //          to identical or similar toolbars in class MusE or other TopWin classes. 
      //         This allows MusE::setCurrentMenuSharingTopwin() to do some magic
      //          to retain the original toolbar layout. If it finds an existing
      //          toolbar with the same object name, it /replaces/ it using insertToolBar(),
      //          instead of /appending/ with addToolBar().

      addToolBarBreak();
      
      // Already has an object name.
      tools2 = new MusEGui::EditToolBar(this, waveEditTools);
      addToolBar(tools2);
      
      tb1 = addToolBar(tr("WaveEdit tools"));
      tb1->setObjectName("Wave Pos/Snap/Solo-tools");

      solo = new QToolButton();
      solo->setText(tr("Solo"));
      solo->setCheckable(true);
      solo->setFocusPolicy(Qt::NoFocus);
      tb1->addWidget(solo);
      connect(solo,  SIGNAL(toggled(bool)), SLOT(soloChanged(bool)));
      
      QLabel* label = new QLabel(tr("Cursor"));
      tb1->addWidget(label);
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      pos1 = new PosLabel(nullptr);
      pos1->setObjectName("PosLabel");
      pos1->setFixedHeight(22);
      tb1->addWidget(pos1);
      pos2 = new PosLabel(nullptr);
      pos2->setObjectName("PosLabel");
      pos2->setFixedHeight(22);
      pos2->setSmpte(true);
      tb1->addWidget(pos2);

      gridOnButton = new QToolButton();
      gridOnButton->setIcon(*gridOnSVGIcon);
      gridOnButton->setFocusPolicy(Qt::NoFocus);
      gridOnButton->setCheckable(true);
      gridOnButton->setToolTip(tr("Show grid"));
      gridOnButton->setWhatsThis(tr("Show grid"));
      tb1->addWidget(gridOnButton);
      connect(gridOnButton, &QToolButton::toggled, [this](bool v) { gridOnChanged(v); } );

      rasterLabel = new RasterLabelCombo(RasterLabelCombo::TableView, _rasterizerModel, this, "RasterLabelCombo");
      rasterLabel->setFocusPolicy(Qt::TabFocus);
      tb1->addWidget(rasterLabel);

      //---------------------------------------------------
      //    Rest
      //---------------------------------------------------

      int yscale = 256;
      int xscale;

      if (!parts()->empty()) { // Roughly match total size of part
            MusECore::Part* firstPart = parts()->begin()->second;
            xscale = 0 - firstPart->lenFrame()/_widthInit[_type];
            }
      else {
            xscale = -8000;
            }

      hscroll = new ScrollScale(
        _minXMag,
        _maxXMag,
        xscale,
        10000,
        Qt::Horizontal,
        mainw,
        0,
        false,
        10000.0);

      canvas    = new WaveCanvas(this, mainw, xscale, yscale);

      QSizeGrip* corner    = new QSizeGrip(mainw);
      ymag                 = new QSlider(Qt::Vertical, mainw);
      ymag->setMinimum(1);
      ymag->setMaximum(256);
      ymag->setPageStep(256);
      ymag->setValue(yscale);
      ymag->setFocusPolicy(Qt::NoFocus);

      //---------------------------------------------------
      //    split
      //---------------------------------------------------

      hsplitter = new MusEGui::Splitter(Qt::Horizontal, mainw, "hsplitter");
      hsplitter->setChildrenCollapsible(true);
      //hsplitter->setHandleWidth(4);
      
      trackInfoWidget = new TrackInfoWidget(hsplitter);
      genTrackInfo(trackInfoWidget);

      time                 = new MTScale(_raster, mainw, xscale, true);
      
      //QWidget* split1     = new QWidget(splitter);
      QWidget* split1     = new QWidget();
      split1->setObjectName("split1");
      QGridLayout* gridS1 = new QGridLayout(split1);
      gridS1->setContentsMargins(0, 0, 0, 0);
      gridS1->setSpacing(0);  

      gridS1->setRowStretch(2, 100);
      gridS1->setColumnStretch(1, 100);     

      gridS1->addWidget(time,   0, 0, 1, 2);
      gridS1->addWidget(MusECore::hLine(mainw),    1, 0, 1, 2);
      gridS1->addWidget(canvas,    2, 0);
      gridS1->addWidget(ymag,    2, 1);
      
      QWidget* gridS2_w = new QWidget();
      gridS2_w->setObjectName("gridS2_w");
      gridS2_w->setContentsMargins(0, 0, 0, 0);
      QGridLayout* gridS2 = new QGridLayout(gridS2_w);
      gridS2->setContentsMargins(0, 0, 0, 0);
      gridS2->setSpacing(0);  
      gridS2->setRowStretch(0, 100);
      //gridS2->setColumnStretch(1, 100);
      //gridS2->addWidget(ctrl,    0, 0);
      gridS2->addWidget(hscroll, 0, 0);
      gridS2->addWidget(corner,  0, 1, Qt::AlignBottom|Qt::AlignRight);
      gridS2_w->setMaximumHeight(hscroll->sizeHint().height());
      gridS2_w->setMinimumHeight(hscroll->sizeHint().height());
      
      QWidget* splitter_w = new QWidget();
      splitter_w->setObjectName("splitter_w");
      splitter_w->setContentsMargins(0, 0, 0, 0);
      QVBoxLayout* splitter_vbox = new QVBoxLayout(splitter_w);
      splitter_vbox->setContentsMargins(0, 0, 0, 0);
      splitter_vbox->setSpacing(0);  
      //splitter_vbox->addWidget(splitter);
      splitter_vbox->addWidget(split1);
      splitter_vbox->addWidget(gridS2_w);
      
      hsplitter->addWidget(splitter_w);
          
      hsplitter->setStretchFactor(hsplitter->indexOf(trackInfoWidget), 0);
      QSizePolicy tipolicy = QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
      tipolicy.setHorizontalStretch(0);
      tipolicy.setVerticalStretch(100);
      trackInfoWidget->setSizePolicy(tipolicy);

      hsplitter->setStretchFactor(hsplitter->indexOf(splitter_w), 1);
      QSizePolicy epolicy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      epolicy.setHorizontalStretch(255);
      epolicy.setVerticalStretch(100);
      //splitter->setSizePolicy(epolicy);
      
      ymag->setFixedWidth(16);
      
      connect(canvas, SIGNAL(mouseWheelMoved(int)), this, SLOT(moveVerticalSlider(int)));
      connect(ymag, SIGNAL(valueChanged(int)), canvas, SLOT(setYScale(int)));
      connect(canvas, SIGNAL(horizontalZoom(bool, const QPoint&)), SLOT(horizontalZoom(bool, const QPoint&)));
      connect(canvas, SIGNAL(horizontalZoom(int, const QPoint&)), SLOT(horizontalZoom(int, const QPoint&)));

      canvas->setOrigin(_canvasXOrigin, 0);
      time->setOrigin(_canvasXOrigin, 0);

      changeRaster(_raster);
      
      
      mainGrid->addWidget(hsplitter, 0, 0, 1, 1);

      canvas->setFocus();  
      
      connect(canvas, SIGNAL(toolChanged(int)), tools2, SLOT(set(int)));
      connect(tools2, SIGNAL(toolChanged(int)), canvas,   SLOT(setTool(int)));
      connect(MusEGlobal::muse, &MusE::configChanged, tools2, &EditToolBar::configChanged);

      connect(hscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)),  canvas, SLOT(setXMag(int)));
      setWindowTitle(canvas->getCaption());
      connect(canvas, SIGNAL(followEvent(int)), hscroll, SLOT(setOffset(int)));

      connect(hscroll, SIGNAL(scrollChanged(int)), time,  SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)),  time,  SLOT(setXMag(int)));
      connect(time,    SIGNAL(timeChanged(unsigned)),  SLOT(timeChanged(unsigned)));
      connect(canvas,    SIGNAL(timeChanged(unsigned)),  SLOT(setTime(unsigned)));

      connect(canvas,  SIGNAL(horizontalScroll(unsigned)),hscroll, SLOT(setPos(unsigned)));
      connect(canvas,  SIGNAL(horizontalScrollNoLimit(unsigned)),hscroll, SLOT(setPosNoLimit(unsigned))); 
      connect(canvas, SIGNAL(curPartHasChanged(MusECore::Part*)), SLOT(updateTrackInfo()));

      connect(hscroll, SIGNAL(scaleChanged(int)),  SLOT(updateHScrollRange()));
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged1(MusECore::SongChangedStruct_t)));

      connect(rasterLabel, &RasterLabelCombo::rasterChanged, [this](int raster) { _setRaster(raster); } );

      // For the wave editor, let's start with the operation range selection tool.
      canvas->setTool(MusEGui::RangeTool);
      tools2->set(MusEGui::RangeTool);
      
      setEventColorMode(colorMode);
      
      initShortcuts();
      
      updateHScrollRange();
      configChanged();
      
      if(!parts()->empty())
      {
        MusECore::WavePart* part = (MusECore::WavePart*)(parts()->begin()->second);
        solo->setChecked(part->track()->solo());
      }

      QList<int> mops;
      mops.append(_trackInfoWidthInit);
      mops.append(_canvasWidthInit);
      hsplitter->setSizes(mops);
    
      if(canvas->track())
      {
        updateTrackInfo();
        //toolbar->setSolo(canvas->track()->solo());
      }
         
      initTopwinState();
      finalizeInit();
      }

void WaveEdit::initShortcuts()
      {
      cutAction->setShortcut(shortcuts[SHRT_CUT].key);
      copyAction->setShortcut(shortcuts[SHRT_COPY].key);
      pasteAction->setShortcut(shortcuts[SHRT_PASTE].key);
      selectAllAction->setShortcut(shortcuts[SHRT_SELECT_ALL].key);
      selectNoneAction->setShortcut(shortcuts[SHRT_SELECT_NONE].key);
      
      //selectInvertAction->setShortcut(shortcuts[SHRT_SELECT_INVERT].key);
      //selectInsideLoopAction->setShortcut(shortcuts[SHRT_SELECT_ILOOP].key);
      //selectOutsideLoopAction->setShortcut(shortcuts[SHRT_SELECT_OLOOP].key);

      if (selectPrevPartAction && selectNextPartAction) {
          selectPrevPartAction->setShortcut(shortcuts[SHRT_SELECT_PREV_PART].key);
          selectNextPartAction->setShortcut(shortcuts[SHRT_SELECT_NEXT_PART].key);
      }

      selectRangeToSelectionAction->setShortcut(shortcuts[SHRT_LOCATORS_TO_SELECTION].key);
      
//      eventColor->menuAction()->setShortcut(shortcuts[SHRT_EVENT_COLOR].key);
      //evColorNormalAction->setShortcut(shortcuts[  ].key);
      //evColorPartsAction->setShortcut(shortcuts[  ].key);
      
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void WaveEdit::configChanged()
      {
      if (MusEGlobal::config.canvasBgPixmap.isEmpty()) {
            canvas->setBg(MusEGlobal::config.waveEditBackgroundColor);
            canvas->setBg(QPixmap());
      }
      else {
            canvas->setBg(QPixmap(MusEGlobal::config.canvasBgPixmap));
      }
      
      gridOnButton->blockSignals(true);
      gridOnButton->setChecked(MusEGlobal::config.canvasShowGrid);
      gridOnButton->blockSignals(false);

      initShortcuts();
      
      canvas->redraw();
      }

//---------------------------------------------------------
//   updateHScrollRange
//---------------------------------------------------------
void WaveEdit::updateHScrollRange()
{
      int s, e;
      canvas->range(&s, &e);   // Range in frames
      unsigned tm = MusEGlobal::sigmap.ticksMeasure(MusEGlobal::tempomap.frame2tick(e));
      
      // Show one more measure, and show another quarter measure due to imprecise drawing at canvas end point.
      //e += MusEGlobal::tempomap.tick2frame(tm + tm / 4);  // TODO: Try changing scrollbar to use units of frames?
      e += (tm + tm / 4);
      
      // Compensate for the vscroll width. 
      //e += wview->rmapxDev(-vscroll->width()); 
      int s1, e1;
      hscroll->range(&s1, &e1);                             //   ...
      if(s != s1 || e != e1) 
        hscroll->setRange(s, e);                            //   ...
}

//---------------------------------------------------------
//   timeChanged
//---------------------------------------------------------

void WaveEdit::timeChanged(unsigned t)
{
      if(t == INT_MAX)
      {
        // Let the PosLabels disable themselves with INT_MAX.
        pos1->setValue(t);
        pos2->setValue(t);
        return;
      }
     
      unsigned frame = MusEGlobal::tempomap.tick2frame(t);
      pos1->setValue(t);
      pos2->setValue(frame);
      time->setPos(3, t, false);
}

//---------------------------------------------------------
//   setTime
//---------------------------------------------------------

void WaveEdit::setTime(unsigned samplepos)
      {
      if(samplepos == INT_MAX)
      {
        // Let the PosLabels disable themselves with INT_MAX.
        pos1->setValue(samplepos);
        pos2->setValue(samplepos);
        return;
      }
     
      // Normally frame to tick methods round down. But here we need it to 'snap'
      //  the frame from either side of a tick to the tick. So round to nearest.
      unsigned tick = MusEGlobal::tempomap.frame2tick(samplepos, 0, MusECore::LargeIntRoundNearest);
      // Rasterize the tick.
      tick = MusEGlobal::sigmap.raster(tick, _raster);
      // To rasterize the pos2 frame, we must convert the rasterized tick back to frames.
      samplepos = MusEGlobal::tempomap.tick2frame(tick);

      pos1->setValue(tick);
      pos2->setValue(samplepos);
      time->setPos(3, tick, false);
      }

//---------------------------------------------------------
//   ~WaveEdit
//---------------------------------------------------------

WaveEdit::~WaveEdit()
      {
      DEBUG_WAVEEDIT(stderr, "WaveEdit dtor\n");
      }

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void WaveEdit::cmd(int n)
      {
      // Don't process if user is dragging or has clicked on the events. 
      // Causes crashes later in Canvas::viewMouseMoveEvent and viewMouseReleaseEvent.
      if(canvas->getCurrentDrag())
        return;
      
      ((WaveCanvas*)canvas)->cmd(n);
      }

//---------------------------------------------------------
//   loadConfiguration
//---------------------------------------------------------

void WaveEdit::readConfiguration(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::TagStart:
                        if (tag == "bgcolor")
                              MusEGlobal::config.waveEditBackgroundColor = readColor(xml);
                        else if (tag == "raster")
                              _rasterInit = xml.parseInt();
                        else if (tag == "trackinfowidth")
                              _trackInfoWidthInit = xml.parseInt();
                        else if (tag == "canvaswidth")
                              _canvasWidthInit = xml.parseInt();
                        else if (tag == "colormode")
                              colorModeInit = xml.parseInt();
                        else if (tag == "topwin")
                              TopWin::readConfiguration(WAVE, xml);
                        else
                              xml.unknown("WaveEdit");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "waveedit")
                              return;
                  default:
                        break;
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  }
            }
      }

//---------------------------------------------------------
//   saveConfiguration
//---------------------------------------------------------

void WaveEdit::writeConfiguration(int level, MusECore::Xml& xml)
      {
      xml.tag(level++, "waveedit");
      xml.colorTag(level, "bgcolor", MusEGlobal::config.waveEditBackgroundColor);
      xml.intTag(level, "raster", _rasterInit);
      xml.intTag(level, "trackinfowidth", _trackInfoWidthInit);
      xml.intTag(level, "canvaswidth", _canvasWidthInit);
      xml.intTag(level, "colormode", colorModeInit);
      TopWin::writeConfiguration(WAVE, level,xml);
      xml.tag(level, "/waveedit");
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void WaveEdit::writeStatus(int level, MusECore::Xml& xml) const
      {
      writePartList(level, xml);
      xml.tag(level++, "waveedit");
      MidiEditor::writeStatus(level, xml);
      xml.intTag(level, "tool", int(canvas->tool()));
      xml.intTag(level, "xmag", hscroll->mag());
      xml.intTag(level, "xpos", hscroll->pos());
      xml.intTag(level, "ymag", ymag->value());
      xml.tag(level, "/waveedit");
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void WaveEdit::readStatus(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::TagStart:
                        if (tag == "midieditor")
                              MidiEditor::readStatus(xml);
                        else if (tag == "tool") {
                              int tool = xml.parseInt();
                              canvas->setTool(tool);
                              tools2->set(tool);
                              }
                        else if (tag == "xmag")
                              hscroll->setMag(xml.parseInt());
                        else if (tag == "ymag")
                              ymag->setValue(xml.parseInt());
                        else if (tag == "xpos")
                              hscroll->setPos(xml.parseInt());
                        else
                              xml.unknown("WaveEdit");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "waveedit") {
                              changeRaster(_raster);
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   songChanged1
//    signal from "song"
//---------------------------------------------------------

void WaveEdit::songChanged1(MusECore::SongChangedStruct_t bits)
      {
        if(_isDeleting)  // Ignore while while deleting to prevent crash.
          return;

        // We must catch this first and be sure to update the strips.
        if(bits & SC_TRACK_REMOVED)
          checkTrackInfoTrack();
        
        if (bits & SC_DIVISION_CHANGED)
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
          changeRaster(_raster);

          // Now set a reasonable zoom (mag) range.
          setupHZoomRange();
        }
        
        if (bits & SC_SOLO)
        {
          MusECore::WavePart* part = (MusECore::WavePart*)(parts()->begin()->second);
          solo->blockSignals(true);
          solo->setChecked(part->track()->solo());
          solo->blockSignals(false);
        }  
        
        songChanged(bits);

        // We'll receive SC_SELECTION if a different part is selected.
        // Addition - also need to respond here to moving part to another track. (Tim)
        if (bits & (SC_PART_INSERTED | SC_PART_REMOVED))
          updateTrackInfo();

        // We must marshall song changed instead of connecting to the strip's song changed
        //  otherwise it crashes when loading another song because track is no longer valid
        //  and the strip's songChanged() seems to be called before Pianoroll songChanged()
        //  gets called and has a chance to stop the crash.
        // Also, calling updateTrackInfo() from here is too heavy, it destroys and recreates
        //  the strips each time no matter what the flags are !
        else  
          trackInfoSongChange(bits);
      }


//---------------------------------------------------------
//   soloChanged
//    signal from solo button
//---------------------------------------------------------

void WaveEdit::soloChanged(bool flag)
      {
      MusECore::WavePart* part = (MusECore::WavePart*)(parts()->begin()->second);
      // This is a minor operation easily manually undoable. Let's not clog the undo list with it.
      MusECore::PendingOperationList operations;
      operations.add(MusECore::PendingOperationItem(part->track(), flag, MusECore::PendingOperationItem::SetTrackSolo));
      MusEGlobal::audio->msgExecutePendingOperations(operations, true);
      }

//---------------------------------------------------------
//   gridOnChanged
//---------------------------------------------------------

void WaveEdit::gridOnChanged(bool v)
{
  MusEGlobal::config.canvasShowGrid = v;
  // We want the simple version, don't set the style or stylesheet yet.
  MusEGlobal::muse->changeConfig(true);
}

//---------------------------------------------------------
//   viewKeyPressEvent
//---------------------------------------------------------

void WaveEdit::keyPressEvent(QKeyEvent* event)
      {
      RasterizerModel::RasterPick rast_pick = RasterizerModel::NoPick;
      const int cur_rast = raster();

      WaveCanvas* wc = (WaveCanvas*)canvas;
      int key = event->key();

      if (((QInputEvent*)event)->modifiers() & Qt::ShiftModifier)
            key += Qt::SHIFT;
      if (((QInputEvent*)event)->modifiers() & Qt::AltModifier)
            key += Qt::ALT;
      if (((QInputEvent*)event)->modifiers() & Qt::ControlModifier)
            key+= Qt::CTRL;

      if (key == Qt::Key_Escape) {
            close();
            return;
            }
            
      else if (key == shortcuts[SHRT_POS_INC].key) {
            wc->waveCmd(CMD_RIGHT);
            return;
            }
      else if (key == shortcuts[SHRT_POS_DEC].key) {
            wc->waveCmd(CMD_LEFT);
            return;
            }
      else if (key == shortcuts[SHRT_POS_INC_NOSNAP].key) {
            wc->waveCmd(CMD_RIGHT_NOSNAP);
            return;
            }
      else if (key == shortcuts[SHRT_POS_DEC_NOSNAP].key) {
            wc->waveCmd(CMD_LEFT_NOSNAP);
            return;
            }
      else if (key == shortcuts[SHRT_INSERT_AT_LOCATION].key) {
            wc->waveCmd(CMD_INSERT);
            return;
            }
      else if (key == Qt::Key_Backspace) {
            wc->waveCmd(CMD_BACKSPACE);
            return;
            }
            
      else if (key == shortcuts[SHRT_TOOL_POINTER].key) {
            tools2->set(MusEGui::PointerTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_PENCIL].key) {
            tools2->set(MusEGui::PencilTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_RUBBER].key) {
            tools2->set(MusEGui::RubberTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_SCISSORS].key) {
            tools2->set(MusEGui::CutTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_PAN].key) {
            tools2->set(MusEGui::PanTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_ZOOM].key) {
            tools2->set(MusEGui::ZoomTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_RANGE].key) {
            tools2->set(MusEGui::RangeTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_STRETCH].key) {
            tools2->set(MusEGui::StretchTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_SAMPLERATE].key) {
            tools2->set(MusEGui::SamplerateTool);
            return;
            }
            
      else if (key == shortcuts[SHRT_EVENT_COLOR].key) {
            if (colorMode == 0)
                  colorMode = 1;
            else if (colorMode == 1)
                  colorMode = 0;
            setEventColorMode(colorMode);
            return;
            }
            
      // TODO: New WaveCanvas: Convert some of these to use frames.
      else if (key == shortcuts[SHRT_ZOOM_IN].key) {
            horizontalZoom(true, QCursor::pos());
            return;
            }
      else if (key == shortcuts[SHRT_ZOOM_OUT].key) {
            horizontalZoom(false, QCursor::pos());
            return;
            }
      else if (key == shortcuts[SHRT_GOTO_CPOS].key) {
            MusECore::PartList* p = this->parts();
            MusECore::Part* first = p->begin()->second;
            hscroll->setPos(MusEGlobal::song->cpos() - first->tick() );
            return;
            }
      else if (key == shortcuts[SHRT_SCROLL_LEFT].key) {
            int pos = hscroll->pos() - MusEGlobal::config.division;
            if (pos < 0)
                  pos = 0;
            hscroll->setPos(pos);
            return;
            }
      else if (key == shortcuts[SHRT_SCROLL_RIGHT].key) {
            int pos = hscroll->pos() + MusEGlobal::config.division;
            hscroll->setPos(pos);
            return;
            }
            
      else if (key == shortcuts[SHRT_SET_QUANT_BAR].key)
            rast_pick = RasterizerModel::GotoBar;
      else if (key == shortcuts[SHRT_SET_QUANT_OFF].key)
            //this hack has the downside that the next shortcut will use triols, but it's better than not having it, I think...
            rast_pick = RasterizerModel::GotoOff;
      else if (key == shortcuts[SHRT_SET_QUANT_1].key)
            rast_pick = RasterizerModel::Goto1;
      else if (key == shortcuts[SHRT_SET_QUANT_2].key)
            rast_pick = RasterizerModel::Goto2;
      else if (key == shortcuts[SHRT_SET_QUANT_3].key)
            rast_pick = RasterizerModel::Goto4;
      else if (key == shortcuts[SHRT_SET_QUANT_4].key)
            rast_pick = RasterizerModel::Goto8;
      else if (key == shortcuts[SHRT_SET_QUANT_5].key)
            rast_pick = RasterizerModel::Goto16;
      else if (key == shortcuts[SHRT_SET_QUANT_6].key)
            rast_pick = RasterizerModel::Goto32;
      else if (key == shortcuts[SHRT_SET_QUANT_7].key)
            rast_pick = RasterizerModel::Goto64;
      else if (key == shortcuts[SHRT_TOGGLE_TRIOL].key)
            rast_pick = RasterizerModel::ToggleTriple;
      else if (key == shortcuts[SHRT_TOGGLE_PUNCT].key)
            rast_pick = RasterizerModel::ToggleDotted;
      else if (key == shortcuts[SHRT_TOGGLE_PUNCT2].key)
            rast_pick = RasterizerModel::ToggleHigherDotted;
            
      else { //Default:
            event->ignore();
            return;
            }
        
      if(rast_pick != RasterizerModel::NoPick)
      {
        const int new_rast = _rasterizerModel->pickRaster(cur_rast, rast_pick);
        if(new_rast != cur_rast)
        {
          setRaster(new_rast);
          //toolbar->setRaster(_raster);
          
          const QModelIndex mdl_idx = _rasterizerModel->modelIndexOfRaster(_raster);
          if(mdl_idx.isValid())
            rasterLabel->setCurrentModelIndex(mdl_idx);
          else
            fprintf(stderr, "WaveEdit::keyPressEvent: _raster %d not found in box!\n", _raster);
        }
      }
      }

//---------------------------------------------------------
//   moveVerticalSlider
//---------------------------------------------------------

void WaveEdit::moveVerticalSlider(int val)
      {
      ymag->setValue(ymag->value() + val);
      }

void WaveEdit::horizontalZoom(bool zoom_in, const QPoint& glob_pos)
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
  QPoint sp = mainw->mapFromGlobal(glob_pos);
  if(cp.x() >= 0 && cp.x() < canvas->width() && sp.y() >= 0 && sp.y() < mainw->height())
    hscroll->setMag(newmag, cp.x());
}

void WaveEdit::horizontalZoom(int mag, const QPoint& glob_pos)
{
  QPoint cp = canvas->mapFromGlobal(glob_pos);
  QPoint sp = mainw->mapFromGlobal(glob_pos);
  if(cp.x() >= 0 && cp.x() < canvas->width() && sp.y() >= 0 && sp.y() < mainw->height())
    hscroll->setMag(hscroll->mag() + mag, cp.x());
}

//---------------------------------------------------------
//   focusCanvas
//---------------------------------------------------------

void WaveEdit::focusCanvas()
{
  if(MusEGlobal::config.smartFocus)
  {
    canvas->setFocus();
    canvas->activateWindow();
  }
}

//---------------------------------------------------------
//   _setRaster
//---------------------------------------------------------

// void WaveEdit::_setRaster(int index)
//       {
//       const int rast = rasterLabel->itemData(index, RasterizerModel::RasterValueRole).toInt();
//       MidiEditor::setRaster(rast);
//       _rasterInit = _raster;
//       time->setRaster(_raster);
//       canvas->redrawGrid();
//       for (auto it : ctrlEditList)
//           it->redrawCanvas();
// 
//       focusCanvas();
//       }

void WaveEdit::_setRaster(int raster)
{
      MidiEditor::setRaster(raster);
      _rasterInit = _raster;
      time->setRaster(_raster);
      canvas->redrawGrid();
      for (auto it : ctrlEditList)
          it->redrawCanvas();
      focusCanvas();
}

//---------------------------------------------------------
//   changeRaster
//---------------------------------------------------------

int WaveEdit::changeRaster(int val)
      {
//         _raster = _rasterizerModel->checkRaster(_raster);
//         _rasterInit = _raster;
        //const int idx = _rasterizerModel->indexOfRaster(_raster);
//         const int idx = rasterLabel->findData(_raster, MusECore::RasterizerModel::RasterValueRole);
//         if(idx >= 0)
//           rasterLabel->setCurrentIndex(idx);
        
        
        
//         const int lb_val = _rlistModel->checkRaster(val);
//         const int idx = _rlistModel->indexOfRaster(lb_val);
//         if(idx >= 0)
//           raster->setCurrentIndex(idx);
//         return lb_val;

//         const int lb_val = _rasterizerModel->checkRaster(val);
        const RasterizerModel* rast_mdl = rasterLabel->rasterizerModel();
        MidiEditor::setRaster(rast_mdl->checkRaster(val));
        _rasterInit = _raster;
        time->setRaster(_raster);
//         const int idx = rasterLabel->findData(_raster, RasterizerModel::RasterValueRole);
//         const int idx = rast_mdl->indexOfRaster(_raster);
//         if(idx >= 0)
//           rasterLabel->setCurrentIndex(idx);
//         else
//           fprintf(stderr, "WaveEdit::changeRaster: _raster %d not found in box!\n", _raster);
        const QModelIndex mdl_idx = rast_mdl->modelIndexOfRaster(_raster);
        if(mdl_idx.isValid())
          rasterLabel->setCurrentModelIndex(mdl_idx);
        else
          fprintf(stderr, "WaveEdit::changeRaster: _raster %d not found in box!\n", _raster);

        canvas->redrawGrid();
        for (auto it : ctrlEditList)
            it->redrawCanvas();
        return _raster;
      }

//---------------------------------------------------------
//   eventColorModeChanged
//---------------------------------------------------------

void WaveEdit::eventColorModeChanged(int mode)
      {
      colorMode = mode;
      colorModeInit = colorMode;
      
      ((WaveCanvas*)(canvas))->setColorMode(colorMode);
      }

//---------------------------------------------------------
//   setEventColorMode
//---------------------------------------------------------

void WaveEdit::setEventColorMode(int mode)
      {
      colorMode = mode;
      colorModeInit = colorMode;
      
      evColorNormalAction->setChecked(mode == 0);
      evColorPartsAction->setChecked(mode == 1);
      
      ((WaveCanvas*)(canvas))->setColorMode(colorMode);
      }

//---------------------------------------------------------
//   setupHZoomRange
//---------------------------------------------------------

void WaveEdit::setupHZoomRange()
{
  //const int min = (_minXMag * MusEGlobal::config.division) / 384;
  //hscroll->setScaleRange(min, _maxXMag);
  hscroll->setScaleRange(_minXMag, _maxXMag);
}

} // namespace MusEGui
