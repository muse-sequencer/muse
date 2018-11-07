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

#include <QMenu>
#include <QSignalMapper>
#include <QToolBar>
#include <QToolButton>
#include <QLayout>
#include <QSizeGrip>
#include <QScrollBar>
#include <QLabel>
#include <QSlider>
#include <QMenuBar>
#include <QAction>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QSettings>
#include <QCursor>
#include <QPoint>
#include <QRect>

#include "app.h"
#include "xml.h"
#include "waveedit.h"
#include "mtscale.h"
#include "scrollscale.h"
//#include "waveview.h"
#include "wavecanvas.h"
#include "ttoolbar.h"
#include "globals.h"
#include "audio.h"
#include "utils.h"
#include "song.h"
#include "poslabel.h"
#include "gconfig.h"
#include "icons.h"
#include "shortcuts.h"
#include "cmd.h"
#include "operations.h"

namespace MusECore {
extern QColor readColor(MusECore::Xml& xml);
}

namespace MusEGui {

static int waveEditTools = MusEGui::PointerTool | MusEGui::PencilTool | MusEGui::RubberTool | 
                           MusEGui::CutTool | MusEGui::RangeTool | PanTool | ZoomTool;

int WaveEdit::_rasterInit = 96;
int WaveEdit::colorModeInit = 0;
  
//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void WaveEdit::closeEvent(QCloseEvent* e)
      {
      _isDeleting = true;  // Set flag so certain signals like songChanged, which may cause crash during delete, can be ignored.
      
      QSettings settings("MusE", "MusE-qt");
      //settings.setValue("Waveedit/geometry", saveGeometry());
      settings.setValue("Waveedit/windowState", saveState());
      emit isDeleting(static_cast<TopWin*>(this));
      e->accept();
      }

//---------------------------------------------------------
//   WaveEdit
//---------------------------------------------------------

WaveEdit::WaveEdit(MusECore::PartList* pl, QWidget* parent, const char* name)
   : MidiEditor(TopWin::WAVE, 1, pl, parent, name)
      {
      setFocusPolicy(Qt::NoFocus);
      colorMode      = colorModeInit;

      QSignalMapper* mapper = new QSignalMapper(this);
      QSignalMapper* colorMapper = new QSignalMapper(this);
      QAction* act;
      
      //---------Pulldown Menu----------------------------
      // We probably don't need an empty menu - Orcan
      //QMenu* menuFile = menuBar()->addMenu(tr("&File"));
      QMenu* menuEdit = menuBar()->addMenu(tr("&Edit"));
      
      menuFunctions = menuBar()->addMenu(tr("Func&tions"));

      menuGain = menuFunctions->addMenu(tr("&Gain"));
      
      act = menuGain->addAction("200%");
      mapper->setMapping(act, WaveCanvas::CMD_GAIN_200);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuGain->addAction("150%");
      mapper->setMapping(act, WaveCanvas::CMD_GAIN_150);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuGain->addAction("75%");
      mapper->setMapping(act, WaveCanvas::CMD_GAIN_75);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuGain->addAction("50%");
      mapper->setMapping(act, WaveCanvas::CMD_GAIN_50);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuGain->addAction("25%");
      mapper->setMapping(act, WaveCanvas::CMD_GAIN_25);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuGain->addAction(tr("Other"));
      mapper->setMapping(act, WaveCanvas::CMD_GAIN_FREE);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      connect(mapper, SIGNAL(mapped(int)), this, SLOT(cmd(int)));
      
      menuFunctions->addSeparator();

      copyAction = menuEdit->addAction(tr("&Copy"));
      mapper->setMapping(copyAction, WaveCanvas::CMD_EDIT_COPY);
      connect(copyAction, SIGNAL(triggered()), mapper, SLOT(map()));

      copyPartRegionAction = menuEdit->addAction(tr("&Create Part from Region"));
      mapper->setMapping(copyPartRegionAction, WaveCanvas::CMD_CREATE_PART_REGION);
      connect(copyPartRegionAction, SIGNAL(triggered()), mapper, SLOT(map()));

      cutAction = menuEdit->addAction(tr("C&ut"));
      mapper->setMapping(cutAction, WaveCanvas::CMD_EDIT_CUT);
      connect(cutAction, SIGNAL(triggered()), mapper, SLOT(map()));

      pasteAction = menuEdit->addAction(tr("&Paste"));
      mapper->setMapping(pasteAction, WaveCanvas::CMD_EDIT_PASTE);
      connect(pasteAction, SIGNAL(triggered()), mapper, SLOT(map()));


      act = menuEdit->addAction(tr("Edit in E&xternal Editor"));
      mapper->setMapping(act, WaveCanvas::CMD_EDIT_EXTERNAL);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      menuEdit->addSeparator();

// REMOVE Tim. Also remove CMD_ADJUST_WAVE_OFFSET and so on...      
//       adjustWaveOffsetAction = menuEdit->addAction(tr("Adjust wave offset..."));
//       mapper->setMapping(adjustWaveOffsetAction, WaveCanvas::CMD_ADJUST_WAVE_OFFSET);
//       connect(adjustWaveOffsetAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuFunctions->addAction(tr("Mute Selection"));
      mapper->setMapping(act, WaveCanvas::CMD_MUTE);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuFunctions->addAction(tr("Normalize Selection"));
      mapper->setMapping(act, WaveCanvas::CMD_NORMALIZE);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuFunctions->addAction(tr("Fade In Selection"));
      mapper->setMapping(act, WaveCanvas::CMD_FADE_IN);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuFunctions->addAction(tr("Fade Out Selection"));
      mapper->setMapping(act, WaveCanvas::CMD_FADE_OUT);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      act = menuFunctions->addAction(tr("Reverse Selection"));
      mapper->setMapping(act, WaveCanvas::CMD_REVERSE);
      connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
      
      select = menuEdit->addMenu(QIcon(*selectIcon), tr("Select"));
      
      selectAllAction = select->addAction(QIcon(*select_allIcon), tr("Select &All"));
      mapper->setMapping(selectAllAction, WaveCanvas::CMD_SELECT_ALL);
      connect(selectAllAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      selectNoneAction = select->addAction(QIcon(*select_allIcon), tr("&Deselect All"));
      mapper->setMapping(selectNoneAction, WaveCanvas::CMD_SELECT_NONE);
      connect(selectNoneAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      select->addSeparator();
      
      selectPrevPartAction = select->addAction(QIcon(*select_all_parts_on_trackIcon), tr("&Previous Part"));
      mapper->setMapping(selectPrevPartAction, WaveCanvas::CMD_SELECT_PREV_PART);
      connect(selectPrevPartAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      selectNextPartAction = select->addAction(QIcon(*select_all_parts_on_trackIcon), tr("&Next Part"));
      mapper->setMapping(selectNextPartAction, WaveCanvas::CMD_SELECT_NEXT_PART);
      connect(selectNextPartAction, SIGNAL(triggered()), mapper, SLOT(map()));

      
      QMenu* settingsMenu = menuBar()->addMenu(tr("Window &Config"));

      eventColor = settingsMenu->addMenu(tr("&Event Color"));      
      
      QActionGroup* actgrp = new QActionGroup(this);
      actgrp->setExclusive(true);
      
      evColorNormalAction = actgrp->addAction(tr("&Part colors"));
      evColorNormalAction->setCheckable(true);
      colorMapper->setMapping(evColorNormalAction, 0);
      
      evColorPartsAction = actgrp->addAction(tr("&Gray"));
      evColorPartsAction->setCheckable(true);
      colorMapper->setMapping(evColorPartsAction, 1);
      
      connect(evColorNormalAction, SIGNAL(triggered()), colorMapper, SLOT(map()));
      connect(evColorPartsAction, SIGNAL(triggered()), colorMapper, SLOT(map()));
      
      eventColor->addActions(actgrp->actions());
      
      connect(colorMapper, SIGNAL(mapped(int)), this, SLOT(eventColorModeChanged(int)));
      
      settingsMenu->addSeparator();
      settingsMenu->addAction(subwinAction);
      settingsMenu->addAction(shareAction);
      settingsMenu->addAction(fullscreenAction);

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
      pos1 = new PosLabel(0);
      pos1->setFixedHeight(22);
      tb1->addWidget(pos1);
      pos2 = new PosLabel(0);
      pos2->setFixedHeight(22);
      pos2->setSmpte(true);
      tb1->addWidget(pos2);

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

      hscroll = new ScrollScale(-32768, 1, xscale, 10000, Qt::Horizontal, mainw, 0, false, 10000.0);
      //view    = new WaveView(this, mainw, xscale, yscale);
      canvas    = new WaveCanvas(this, mainw, xscale, yscale);
      //wview   = canvas;   // HACK!

      QSizeGrip* corner    = new QSizeGrip(mainw);
      ymag                 = new QSlider(Qt::Vertical, mainw);
      ymag->setMinimum(1);
      ymag->setMaximum(256);
      ymag->setPageStep(256);
      ymag->setValue(yscale);
      ymag->setFocusPolicy(Qt::NoFocus);

      time                 = new MTScale(&_raster, mainw, xscale, true);
      ymag->setFixedWidth(16);
      connect(canvas, SIGNAL(mouseWheelMoved(int)), this, SLOT(moveVerticalSlider(int)));
      connect(ymag, SIGNAL(valueChanged(int)), canvas, SLOT(setYScale(int)));
      connect(canvas, SIGNAL(horizontalZoom(bool, const QPoint&)), SLOT(horizontalZoom(bool, const QPoint&)));
      connect(canvas, SIGNAL(horizontalZoom(int, const QPoint&)), SLOT(horizontalZoom(int, const QPoint&)));

      time->setOrigin(0, 0);

      mainGrid->setRowStretch(0, 100);
      mainGrid->setColumnStretch(0, 100);

      mainGrid->addWidget(time,   0, 0, 1, 2);
      mainGrid->addWidget(MusECore::hLine(mainw),    1, 0, 1, 2);
      mainGrid->addWidget(canvas,    2, 0);
      mainGrid->addWidget(ymag,    2, 1);
      mainGrid->addWidget(hscroll, 3, 0);
      mainGrid->addWidget(corner,  3, 1, Qt::AlignBottom | Qt::AlignRight);

      canvas->setFocus();  
      
      connect(canvas, SIGNAL(toolChanged(int)), tools2, SLOT(set(int)));
      connect(tools2, SIGNAL(toolChanged(int)), canvas,   SLOT(setTool(int)));
      
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

      connect(hscroll, SIGNAL(scaleChanged(int)),  SLOT(updateHScrollRange()));
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged1(MusECore::SongChangedStruct_t)));

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
      selectPrevPartAction->setShortcut(shortcuts[SHRT_SELECT_PREV_PART].key);
      selectNextPartAction->setShortcut(shortcuts[SHRT_SELECT_NEXT_PART].key);
      
      eventColor->menuAction()->setShortcut(shortcuts[SHRT_EVENT_COLOR].key);
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
      
      initShortcuts();
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
     
      unsigned tick = MusEGlobal::tempomap.frame2tick(samplepos);
      pos1->setValue(tick);
      pos2->setValue(samplepos);
      time->setPos(3, tick, false);
      }

//---------------------------------------------------------
//   ~WaveEdit
//---------------------------------------------------------

WaveEdit::~WaveEdit()
      {
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
                        if (tag == "waveedit")
                              return;
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

        if (bits._flags & SC_SOLO)
        {
          MusECore::WavePart* part = (MusECore::WavePart*)(parts()->begin()->second);
          solo->blockSignals(true);
          solo->setChecked(part->track()->solo());
          solo->blockSignals(false);
        }  
        
        songChanged(bits);
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
//   viewKeyPressEvent
//---------------------------------------------------------

void WaveEdit::keyPressEvent(QKeyEvent* event)
      {
// TODO: Raster:
//       int index;
//       int n = sizeof(rasterTable)/sizeof(*rasterTable);
//       for (index = 0; index < n; ++index)
//             if (rasterTable[index] == raster())
//                   break;
//       if (index == n) {
//             index = 0;
//             // raster 1 is not in table
//             }
//       int off = (index / 9) * 9;
//       index   = index % 9;

//       int val = 0;

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
            
// TODO: Raster:            
//       else if (key == shortcuts[SHRT_SET_QUANT_1].key)
//             val = rasterTable[8 + off];
//       else if (key == shortcuts[SHRT_SET_QUANT_2].key)
//             val = rasterTable[7 + off];
//       else if (key == shortcuts[SHRT_SET_QUANT_3].key)
//             val = rasterTable[6 + off];
//       else if (key == shortcuts[SHRT_SET_QUANT_4].key)
//             val = rasterTable[5 + off];
//       else if (key == shortcuts[SHRT_SET_QUANT_5].key)
//             val = rasterTable[4 + off];
//       else if (key == shortcuts[SHRT_SET_QUANT_6].key)
//             val = rasterTable[3 + off];
//       else if (key == shortcuts[SHRT_SET_QUANT_7].key)
//             val = rasterTable[2 + off];
//       else if (key == shortcuts[SHRT_TOGGLE_TRIOL].key)
//             val = rasterTable[index + ((off == 0) ? 9 : 0)];
//       else if (key == shortcuts[SHRT_TOGGLE_PUNCT].key)
//             val = rasterTable[index + ((off == 18) ? 9 : 18)];
//       else if (key == shortcuts[SHRT_TOGGLE_PUNCT2].key) {//CDW
//             if ((off == 18) && (index > 2)) {
//                   val = rasterTable[index + 9 - 1];
//                   }
//             else if ((off == 9) && (index < 8)) {
//                   val = rasterTable[index + 18 + 1];
//                   }
//             else
//                   return;
//             }
            
      else { //Default:
            event->ignore();
            return;
            }
        
      // TODO: Raster:  
      //setRaster(val);
      //toolbar->setRaster(_raster);
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



} // namespace MusEGui
