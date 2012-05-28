//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pianoroll.cpp,v 1.25.2.15 2009/11/16 11:29:33 lunar_shuttle Exp $
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

#include <QLayout>
#include <QSizeGrip>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QToolTip>
#include <QMenu>
#include <QSignalMapper>
#include <QMenuBar>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QAction>
#include <QKeySequence>
#include <QKeyEvent>
#include <QGridLayout>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QMimeData>
#include <QScrollArea>
#include <QSettings>

#include <stdio.h>

#include "xml.h"
#include "mtscale.h"
#include "prcanvas.h"
#include "pianoroll.h"
#include "scrollscale.h"
#include "piano.h"
#include "../ctrl/ctrledit.h"
#include "splitter.h"
#include "ttoolbar.h"
#include "tb1.h"
#include "utils.h"
#include "globals.h"
#include "app.h"
#include "gconfig.h"
#include "icons.h"
#include "audio.h"
#include "functions.h"
#include "helper.h"


#include "cmd.h"
#include "shortcuts.h"

#include "mtrackinfo.h"

namespace MusEGui {

int PianoRoll::_rasterInit = 96;
int PianoRoll::colorModeInit = 0;

static const int xscale = -10;
static const int yscale = 1;
static const int pianoWidth = 40;
static int pianorollTools = MusEGui::PointerTool | MusEGui::PencilTool | MusEGui::RubberTool | MusEGui::DrawTool;


//---------------------------------------------------------
//   PianoRoll
//---------------------------------------------------------

PianoRoll::PianoRoll(MusECore::PartList* pl, QWidget* parent, const char* name, unsigned initPos)
   : MidiEditor(TopWin::PIANO_ROLL, _rasterInit, pl, parent, name)
      {
      deltaMode     = false;
      tickValue     = 0;
      lenValue      = 0;
      pitchValue    = 0;
      veloOnValue   = 0;
      veloOffValue  = 0;
      firstValueSet = false;
      tickOffset    = 0;
      lenOffset     = 0;
      pitchOffset   = 0;
      veloOnOffset  = 0;
      veloOffOffset = 0;
      lastSelections = 0;
      _playEvents    = false;
      colorMode      = colorModeInit;
      
      QSignalMapper* mapper = new QSignalMapper(this);
      QSignalMapper* colorMapper = new QSignalMapper(this);
      
      //---------Menu----------------------------------
      
      menuEdit = menuBar()->addMenu(tr("&Edit"));      
      
      menuEdit->addActions(MusEGlobal::undoRedo->actions());
      
      menuEdit->addSeparator();
      
      editCutAction = menuEdit->addAction(QIcon(*editcutIconSet), tr("C&ut"));
      mapper->setMapping(editCutAction, PianoCanvas::CMD_CUT);
      connect(editCutAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      editCopyAction = menuEdit->addAction(QIcon(*editcopyIconSet), tr("&Copy"));
      mapper->setMapping(editCopyAction, PianoCanvas::CMD_COPY);
      connect(editCopyAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      editCopyRangeAction = menuEdit->addAction(QIcon(*editcopyIconSet), tr("Copy events in range"));
      mapper->setMapping(editCopyRangeAction, PianoCanvas::CMD_COPY_RANGE);
      connect(editCopyRangeAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      editPasteAction = menuEdit->addAction(QIcon(*editpasteIconSet), tr("&Paste"));
      mapper->setMapping(editPasteAction, PianoCanvas::CMD_PASTE);
      connect(editPasteAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      editPasteDialogAction = menuEdit->addAction(QIcon(*editpasteIconSet), tr("Paste (with dialog)"));
      mapper->setMapping(editPasteDialogAction, PianoCanvas::CMD_PASTE_DIALOG);
      connect(editPasteDialogAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      menuEdit->addSeparator();
      
      editDelEventsAction = menuEdit->addAction(tr("Delete &Events"));
      mapper->setMapping(editDelEventsAction, PianoCanvas::CMD_DEL);
      connect(editDelEventsAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      menuEdit->addSeparator();

      menuSelect = menuEdit->addMenu(QIcon(*selectIcon), tr("&Select"));

      selectAllAction = menuSelect->addAction(QIcon(*select_allIcon), tr("Select &All"));
      mapper->setMapping(selectAllAction, PianoCanvas::CMD_SELECT_ALL);
      connect(selectAllAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      selectNoneAction = menuSelect->addAction(QIcon(*select_deselect_allIcon), tr("&Deselect All"));
      mapper->setMapping(selectNoneAction, PianoCanvas::CMD_SELECT_NONE);
      connect(selectNoneAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      selectInvertAction = menuSelect->addAction(QIcon(*select_invert_selectionIcon), tr("Invert &Selection"));
      mapper->setMapping(selectInvertAction, PianoCanvas::CMD_SELECT_INVERT);
      connect(selectInvertAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      menuSelect->addSeparator();
      
      selectInsideLoopAction = menuSelect->addAction(QIcon(*select_inside_loopIcon), tr("&Inside Loop"));
      mapper->setMapping(selectInsideLoopAction, PianoCanvas::CMD_SELECT_ILOOP);
      connect(selectInsideLoopAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      selectOutsideLoopAction = menuSelect->addAction(QIcon(*select_outside_loopIcon), tr("&Outside Loop"));
      mapper->setMapping(selectOutsideLoopAction, PianoCanvas::CMD_SELECT_OLOOP);
      connect(selectOutsideLoopAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      menuSelect->addSeparator();
      
      selectPrevPartAction = menuSelect->addAction(QIcon(*select_all_parts_on_trackIcon), tr("&Previous Part"));
      mapper->setMapping(selectPrevPartAction, PianoCanvas::CMD_SELECT_PREV_PART);
      connect(selectPrevPartAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      selectNextPartAction = menuSelect->addAction(QIcon(*select_all_parts_on_trackIcon), tr("&Next Part"));
      mapper->setMapping(selectNextPartAction, PianoCanvas::CMD_SELECT_NEXT_PART);
      connect(selectNextPartAction, SIGNAL(triggered()), mapper, SLOT(map()));





      menuFunctions = menuBar()->addMenu(tr("Fu&nctions"));

      menuFunctions->setTearOffEnabled(true);
      
      funcQuantizeAction = menuFunctions->addAction(tr("Quantize"));
      mapper->setMapping(funcQuantizeAction, PianoCanvas::CMD_QUANTIZE);
      connect(funcQuantizeAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      funcGateTimeAction = menuFunctions->addAction(tr("Modify Note Length"));
      mapper->setMapping(funcGateTimeAction, PianoCanvas::CMD_MODIFY_GATE_TIME);
      connect(funcGateTimeAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      funcModVelAction = menuFunctions->addAction(tr("Modify Velocity"));
      mapper->setMapping(funcModVelAction, PianoCanvas::CMD_MODIFY_VELOCITY);
      connect(funcModVelAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      funcCrescAction = menuFunctions->addAction(tr("Crescendo/Decrescendo"));
      mapper->setMapping(funcCrescAction, PianoCanvas::CMD_CRESCENDO);
      connect(funcCrescAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      funcTransposeAction = menuFunctions->addAction(tr("Transpose"));
      mapper->setMapping(funcTransposeAction, PianoCanvas::CMD_TRANSPOSE);
      connect(funcTransposeAction, SIGNAL(triggered()), mapper, SLOT(map()));
            
      funcEraseEventAction = menuFunctions->addAction(tr("Erase Events"));
      mapper->setMapping(funcEraseEventAction, PianoCanvas::CMD_ERASE_EVENT);
      connect(funcEraseEventAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      funcNoteShiftAction = menuFunctions->addAction(tr("Move Notes"));
      mapper->setMapping(funcNoteShiftAction, PianoCanvas::CMD_NOTE_SHIFT);
      connect(funcNoteShiftAction, SIGNAL(triggered()), mapper, SLOT(map()));
            
      funcSetFixedLenAction = menuFunctions->addAction(tr("Set Fixed Length"));
      mapper->setMapping(funcSetFixedLenAction, PianoCanvas::CMD_FIXED_LEN);
      connect(funcSetFixedLenAction, SIGNAL(triggered()), mapper, SLOT(map()));
      
      funcDelOverlapsAction = menuFunctions->addAction(tr("Delete Overlaps"));
      mapper->setMapping(funcDelOverlapsAction, PianoCanvas::CMD_DELETE_OVERLAPS);
      connect(funcDelOverlapsAction, SIGNAL(triggered()), mapper, SLOT(map()));

      QAction* funcLegatoAction = menuFunctions->addAction(tr("Legato"));
      mapper->setMapping(funcLegatoAction, PianoCanvas::CMD_LEGATO);
      connect(funcLegatoAction, SIGNAL(triggered()), mapper, SLOT(map()));
            
                  
      menuPlugins = menuBar()->addMenu(tr("&Plugins"));
      MusEGlobal::song->populateScriptMenu(menuPlugins, this);

      connect(mapper, SIGNAL(mapped(int)), this, SLOT(cmd(int)));
      

      
      
      
      menuConfig = menuBar()->addMenu(tr("Window &Config"));      
      
      eventColor = menuConfig->addMenu(tr("&Event Color"));      
      
      QActionGroup* actgrp = new QActionGroup(this);
      actgrp->setExclusive(true);
      
      evColorBlueAction = actgrp->addAction(tr("&Blue"));
      evColorBlueAction->setCheckable(true);
      colorMapper->setMapping(evColorBlueAction, 0);
      
      evColorPitchAction = actgrp->addAction(tr("&Pitch colors"));
      evColorPitchAction->setCheckable(true);
      colorMapper->setMapping(evColorPitchAction, 1);
      
      evColorVelAction = actgrp->addAction(tr("&Velocity colors"));
      evColorVelAction->setCheckable(true);
      colorMapper->setMapping(evColorVelAction, 2);
      
      connect(evColorBlueAction, SIGNAL(triggered()), colorMapper, SLOT(map()));
      connect(evColorPitchAction, SIGNAL(triggered()), colorMapper, SLOT(map()));
      connect(evColorVelAction, SIGNAL(triggered()), colorMapper, SLOT(map()));
      
      eventColor->addActions(actgrp->actions());
      
      connect(colorMapper, SIGNAL(mapped(int)), this, SLOT(eventColorModeChanged(int)));
      
      menuConfig->addSeparator();
      menuConfig->addAction(subwinAction);
      menuConfig->addAction(shareAction);
      menuConfig->addAction(fullscreenAction);

      
      //---------ToolBar----------------------------------
      tools = addToolBar(tr("Pianoroll tools"));
      tools->setObjectName("Pianoroll tools");

      srec  = new QToolButton();
      srec->setToolTip(tr("Step Record"));
      srec->setIcon(*steprecIcon);
      srec->setCheckable(true);
      srec->setFocusPolicy(Qt::NoFocus);
      tools->addWidget(srec);

      midiin  = new QToolButton();
      midiin->setToolTip(tr("Midi Input"));
      midiin->setIcon(*midiinIcon);
      midiin->setCheckable(true);
      midiin->setFocusPolicy(Qt::NoFocus);
      tools->addWidget(midiin);

      speaker  = new QToolButton();
      speaker->setToolTip(tr("Play Events"));
      speaker->setIcon(*speakerIcon);
      speaker->setCheckable(true);
      speaker->setFocusPolicy(Qt::NoFocus);
      tools->addWidget(speaker);

      tools2 = new MusEGui::EditToolBar(this, pianorollTools);
      addToolBar(tools2);

      addToolBarBreak();
      toolbar = new MusEGui::Toolbar1(this, _rasterInit);
      addToolBar(toolbar);

      addToolBarBreak();
      info    = new MusEGui::NoteInfo(this);
      addToolBar(info);

      //---------------------------------------------------
      //    split
      //---------------------------------------------------

      splitter = new MusEGui::Splitter(Qt::Vertical, mainw, "splitter");
      splitter->setHandleWidth(2);  
      
      hsplitter = new MusEGui::Splitter(Qt::Horizontal, mainw, "hsplitter");
      hsplitter->setChildrenCollapsible(true);
      hsplitter->setHandleWidth(2);
      
      QPushButton* ctrl = new QPushButton(tr("ctrl"), mainw);
      ctrl->setObjectName("Ctrl");
      ctrl->setFont(MusEGlobal::config.fonts[3]);
      ctrl->setToolTip(tr("Add Controller View"));
      ctrl->setFocusPolicy(Qt::NoFocus);
      // Increased scale to -1. To resolve/select/edit 1-tick-wide (controller graph) events. 
      hscroll = new MusEGui::ScrollScale(-25, -1 /* formerly -2 */, xscale, 20000, Qt::Horizontal, mainw);
      ctrl->setFixedSize(pianoWidth, hscroll->sizeHint().height());
      //ctrl->setFixedSize(pianoWidth / 2, hscroll->sizeHint().height());  // DELETETHIS?
      
      QSizeGrip* corner = new QSizeGrip(mainw);

      midiTrackInfo       = new MusEGui::MidiTrackInfo(mainw);        
      int mtiw = midiTrackInfo->width(); // Save this.
      midiTrackInfo->setMinimumWidth(100);   
      //midiTrackInfo->setMaximumWidth(150);   DELETETHIS ?

      midiTrackInfo->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding));
      infoScroll          = new QScrollArea;
      infoScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);  
      infoScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded); 
      infoScroll->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding));
      infoScroll->setWidget(midiTrackInfo);
      infoScroll->setWidgetResizable(true);
      infoScroll->setFocusPolicy(Qt::NoFocus);
      //infoScroll->setVisible(false); DELETETHIS 4?
      //infoScroll->setEnabled(false);

      //hsplitter->addWidget(midiTrackInfo);
      hsplitter->addWidget(infoScroll);  // Tim.
      hsplitter->addWidget(splitter);
          
      mainGrid->setRowStretch(0, 100);
      mainGrid->setColumnStretch(1, 100);
      mainGrid->addWidget(hsplitter, 0, 1, 1, 3);
      
      QWidget* split1     = new QWidget(splitter);
      split1->setObjectName("split1");
      QGridLayout* gridS1 = new QGridLayout(split1);
      gridS1->setContentsMargins(0, 0, 0, 0);
      gridS1->setSpacing(0);  

      time                = new MusEGui::MTScale(&_raster, split1, xscale);
      Piano* piano        = new Piano(split1, yscale);
      canvas              = new PianoCanvas(this, split1, xscale, yscale);
      vscroll             = new MusEGui::ScrollScale(-3, 7, yscale, KH * 75, Qt::Vertical, split1);
      
      int offset = -(MusEGlobal::config.division/4);
      canvas->setOrigin(offset, 0);
      canvas->setCanvasTools(pianorollTools);
      canvas->setFocus();
      connect(canvas, SIGNAL(toolChanged(int)), tools2, SLOT(set(int)));
      connect(canvas, SIGNAL(horizontalZoomIn()), SLOT(horizontalZoomIn()));
      connect(canvas, SIGNAL(horizontalZoomOut()), SLOT(horizontalZoomOut()));
      time->setOrigin(offset, 0);

      gridS1->setRowStretch(2, 100);
      gridS1->setColumnStretch(1, 100);     

      gridS1->addWidget(time,                   0, 1, 1, 2);
      gridS1->addWidget(MusECore::hLine(split1),          1, 0, 1, 3);
      gridS1->addWidget(piano,                  2,    0);
      gridS1->addWidget(canvas,                 2,    1);
      gridS1->addWidget(vscroll,                2,    2);

      ctrlLane = new MusEGui::Splitter(Qt::Vertical, splitter, "ctrllane");
      QWidget* split2     = new QWidget(splitter);
          split2->setMaximumHeight(hscroll->sizeHint().height());
          split2->setMinimumHeight(hscroll->sizeHint().height());
      QGridLayout* gridS2 = new QGridLayout(split2);
      gridS2->setContentsMargins(0, 0, 0, 0);
      gridS2->setSpacing(0);  
      gridS2->setRowStretch(0, 100);
      gridS2->setColumnStretch(1, 100);
          gridS2->addWidget(ctrl,    0, 0);
      gridS2->addWidget(hscroll, 0, 1);
      gridS2->addWidget(corner,  0, 2, Qt::AlignBottom|Qt::AlignRight);
      
      piano->setFixedWidth(pianoWidth);

      QList<int> mops;
      mops.append(mtiw + 30);  // 30 for possible scrollbar
      mops.append(width() - mtiw - 30);
      hsplitter->setSizes(mops);
      
      connect(tools2, SIGNAL(toolChanged(int)), canvas,   SLOT(setTool(int)));

      connect(ctrl, SIGNAL(clicked()), SLOT(addCtrl()));
      connect(info, SIGNAL(valueChanged(MusEGui::NoteInfo::ValType, int)), SLOT(noteinfoChanged(MusEGui::NoteInfo::ValType, int)));
      connect(info, SIGNAL(deltaModeChanged(bool)), SLOT(deltaModeChanged(bool)));

      connect(vscroll, SIGNAL(scrollChanged(int)), piano,  SLOT(setYPos(int)));
      connect(vscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setYPos(int)));
      connect(vscroll, SIGNAL(scaleChanged(int)),  canvas, SLOT(setYMag(int)));
      connect(vscroll, SIGNAL(scaleChanged(int)),  piano,  SLOT(setYMag(int)));

      connect(hscroll, SIGNAL(scrollChanged(int)), canvas,   SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scrollChanged(int)), time,     SLOT(setXPos(int)));

      connect(hscroll, SIGNAL(scaleChanged(int)),  canvas,   SLOT(setXMag(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)),  time,     SLOT(setXMag(int)));

      connect(canvas, SIGNAL(newWidth(int)), SLOT(newCanvasWidth(int)));
      connect(canvas, SIGNAL(pitchChanged(int)), piano, SLOT(setPitch(int)));   
      connect(canvas, SIGNAL(verticalScroll(unsigned)), vscroll, SLOT(setPos(unsigned)));
      connect(canvas,  SIGNAL(horizontalScroll(unsigned)),hscroll, SLOT(setPos(unsigned)));
      connect(canvas,  SIGNAL(horizontalScrollNoLimit(unsigned)),hscroll, SLOT(setPosNoLimit(unsigned))); 
      connect(canvas, SIGNAL(selectionChanged(int, MusECore::Event&, MusECore::Part*, bool)), this,
         SLOT(setSelection(int, MusECore::Event&, MusECore::Part*, bool)));

      connect(piano, SIGNAL(keyPressed(int, int, bool)), canvas, SLOT(pianoPressed(int, int, bool)));
      connect(piano, SIGNAL(keyReleased(int, bool)), canvas, SLOT(pianoReleased(int, bool)));
      connect(srec, SIGNAL(toggled(bool)), SLOT(setSteprec(bool)));
      connect(midiin, SIGNAL(toggled(bool)), canvas, SLOT(setMidiin(bool)));
      connect(speaker, SIGNAL(toggled(bool)), SLOT(setSpeaker(bool)));
      connect(canvas, SIGNAL(followEvent(int)), SLOT(follow(int)));

      connect(info, SIGNAL(returnPressed()),          SLOT(focusCanvas()));
      connect(info, SIGNAL(escapePressed()),          SLOT(focusCanvas()));
      connect(midiTrackInfo, SIGNAL(returnPressed()), SLOT(focusCanvas()));
      connect(midiTrackInfo, SIGNAL(escapePressed()), SLOT(focusCanvas()));

      connect(hscroll, SIGNAL(scaleChanged(int)),  SLOT(updateHScrollRange()));
      piano->setYPos(KH * 30);
      canvas->setYPos(KH * 30);
      vscroll->setPos(KH * 30);
      info->setEnabled(false);

      connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged1(int)));

      setWindowTitle(canvas->getCaption());
      
      updateHScrollRange();
      // connect to toolbar
      connect(canvas,   SIGNAL(pitchChanged(int)), toolbar, SLOT(setPitch(int)));  
      connect(canvas,   SIGNAL(timeChanged(unsigned)),  SLOT(setTime(unsigned)));
      connect(piano,    SIGNAL(pitchChanged(int)), toolbar, SLOT(setPitch(int)));
      connect(time,     SIGNAL(timeChanged(unsigned)),  SLOT(setTime(unsigned)));
      connect(toolbar,  SIGNAL(rasterChanged(int)),SLOT(setRaster(int)));
      connect(toolbar,  SIGNAL(soloChanged(bool)), SLOT(soloChanged(bool)));

      setFocusPolicy(Qt::NoFocus);
      
      setEventColorMode(colorMode);

      QClipboard* cb = QApplication::clipboard();
      connect(cb, SIGNAL(dataChanged()), SLOT(clipboardChanged()));

      clipboardChanged(); // enable/disable "Paste"
      selectionChanged(); // enable/disable "Copy" & "Paste"
      initShortcuts(); // initialize shortcuts

      const MusECore::Pos cpos=MusEGlobal::song->cPos();
      canvas->setPos(0, cpos.tick(), true);
      canvas->selectAtTick(cpos.tick());
        
      unsigned pos=0;
      if(initPos >= INT_MAX)
        pos = MusEGlobal::song->cpos();
      if(pos > INT_MAX)
        pos = INT_MAX;
      if (pos)
        hscroll->setOffset((int)pos);

      if(canvas->track())
      {
        updateTrackInfo();
        toolbar->setSolo(canvas->track()->solo());
      }

      initTopwinState();
      finalizeInit();
      }

//---------------------------------------------------------
//   songChanged1
//---------------------------------------------------------

void PianoRoll::songChanged1(int bits)
      {
        if(_isDeleting)  // Ignore while while deleting to prevent crash.
          return;
        
        if (bits & SC_SOLO)
            toolbar->setSolo(canvas->track()->solo());
        
        songChanged(bits);

        // We'll receive SC_SELECTION if a different part is selected.
        if (bits & SC_SELECTION)
          updateTrackInfo();  
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void PianoRoll::configChanged()
      {
      initShortcuts();
      }

//---------------------------------------------------------
//   updateHScrollRange
//---------------------------------------------------------

void PianoRoll::updateHScrollRange()
{
      int s, e;
      canvas->range(&s, &e);
      // Show one more measure.
      e += AL::sigmap.ticksMeasure(e);  
      // Show another quarter measure due to imprecise drawing at canvas end point.
      e += AL::sigmap.ticksMeasure(e) / 4;
      // Compensate for the fixed piano and vscroll widths. 
      e += canvas->rmapxDev(pianoWidth - vscroll->width()); 
      int s1, e1;
      hscroll->range(&s1, &e1);
      if(s != s1 || e != e1) 
        hscroll->setRange(s, e);
}

void PianoRoll::updateTrackInfo()  
{
      selected = curCanvasPart()->track();
      if (selected->isMidiTrack()) {
            midiTrackInfo->setTrack(selected);
      }
}

//---------------------------------------------------------
//   follow
//---------------------------------------------------------

void PianoRoll::follow(int pos)
      {
      int s, e;
      canvas->range(&s, &e);

      if (pos < e && pos >= s)
            hscroll->setOffset(pos);
      if (pos < s)
            hscroll->setOffset(s);
      }

//---------------------------------------------------------
//   setTime
//---------------------------------------------------------

void PianoRoll::setTime(unsigned tick)
      {
      toolbar->setTime(tick);                      
      time->setPos(3, tick, false);               
      }

//---------------------------------------------------------
//   ~Pianoroll
//---------------------------------------------------------

PianoRoll::~PianoRoll()
      {
      }

//---------------------------------------------------------
//   cmd
//    pulldown menu commands
//---------------------------------------------------------

void PianoRoll::cmd(int cmd)
      {
			switch (cmd)
						{
            case PianoCanvas::CMD_CUT:
                  copy_notes(partlist_to_set(parts()), 1);
                  erase_notes(partlist_to_set(parts()), 1);
                  break;
            case PianoCanvas::CMD_COPY: copy_notes(partlist_to_set(parts()), 1); break;
            case PianoCanvas::CMD_COPY_RANGE: copy_notes(partlist_to_set(parts()), MusECore::any_event_selected(partlist_to_set(parts())) ? 3 : 2); break;
            case PianoCanvas::CMD_PASTE: 
                              ((PianoCanvas*)canvas)->cmd(PianoCanvas::CMD_SELECT_NONE);
                              MusECore::paste_notes(3072);
                              break;
            case PianoCanvas::CMD_PASTE_DIALOG: 
                              ((PianoCanvas*)canvas)->cmd(PianoCanvas::CMD_SELECT_NONE);
                              MusECore::paste_notes((canvas->part()));
                              break;
						case PianoCanvas::CMD_MODIFY_GATE_TIME: modify_notelen(partlist_to_set(parts())); break;
						case PianoCanvas::CMD_MODIFY_VELOCITY: modify_velocity(partlist_to_set(parts())); break;
						case PianoCanvas::CMD_CRESCENDO: crescendo(partlist_to_set(parts())); break;
						case PianoCanvas::CMD_QUANTIZE: quantize_notes(partlist_to_set(parts())); break;
						case PianoCanvas::CMD_TRANSPOSE: transpose_notes(partlist_to_set(parts())); break;
						case PianoCanvas::CMD_ERASE_EVENT: erase_notes(partlist_to_set(parts())); break;
						case PianoCanvas::CMD_DEL: erase_notes(partlist_to_set(parts()),1); break;
						case PianoCanvas::CMD_NOTE_SHIFT: move_notes(partlist_to_set(parts())); break;
						case PianoCanvas::CMD_FIXED_LEN: set_notelen(partlist_to_set(parts())); break;
						case PianoCanvas::CMD_DELETE_OVERLAPS: delete_overlaps(partlist_to_set(parts())); break;
						case PianoCanvas::CMD_LEGATO: legato(partlist_to_set(parts())); break;
						
						default: ((PianoCanvas*)canvas)->cmd(cmd);
					  }
      }

//---------------------------------------------------------
//   setSelection
//    update Info Line
//---------------------------------------------------------

void PianoRoll::setSelection(int tick, MusECore::Event& e, MusECore::Part* /*part*/, bool update)
      {
      int selections = canvas->selectionSize();

      // Diagnostics:
      //printf("PianoRoll::setSelection selections:%d event empty:%d firstValueSet:%d\n", selections, e.empty(), firstValueSet); 
      //if(!e.empty())
      //  e.dump(); 
      
      if(update)
      {
        // Selections have changed. Reset these.
        tickOffset    = 0;
        lenOffset     = 0;
        pitchOffset   = 0;
        veloOnOffset  = 0;
        veloOffOffset = 0;
        
        // Force 'suggested' modes:
        if (selections == 1) 
        {
          deltaMode = false;
          info->setDeltaMode(deltaMode); 
        }
        else
        if (selections > 1)
        {
          // A feeble attempt to hold on to the user's setting. Should try to bring back 
          //  selEvent (removed), but there were problems using it (it's a reference).
          //if(lastSelections <= 1) 
          {
            deltaMode = true;
            info->setDeltaMode(deltaMode); 
          }  
        }
      }

      lastSelections = selections;
      
      if ((selections == 1) || (selections > 1 && !firstValueSet)) 
      {
        tickValue    = tick; 
        lenValue     = e.lenTick();
        pitchValue   = e.pitch();
        veloOnValue  = e.velo();
        veloOffValue = e.veloOff();
        firstValueSet = true;
      }
      
      if (selections > 0) {
            info->setEnabled(true);
            if (deltaMode) 
              info->setValues(tickOffset, lenOffset, pitchOffset, veloOnOffset, veloOffOffset);
            else  
              info->setValues(tickValue, lenValue, pitchValue, veloOnValue, veloOffValue);
            }
      else {
            info->setEnabled(false);
            info->setValues(0, 0, 0, 0, 0);
            firstValueSet = false;
            tickValue     = 0;
            lenValue      = 0;
            pitchValue    = 0;
            veloOnValue   = 0;
            veloOffValue  = 0;
            tickOffset    = 0;
            lenOffset     = 0;
            pitchOffset   = 0;
            veloOnOffset  = 0;
            veloOffOffset = 0;
            }
            
      info->setReturnMode(selections >= 2);      
      selectionChanged();
      }

//---------------------------------------------------------
//   focusCanvas
//---------------------------------------------------------

void PianoRoll::focusCanvas()
{
  if(MusEGlobal::config.smartFocus)
  {
    canvas->setFocus();
    canvas->activateWindow();
  }
}

//---------------------------------------------------------
//   deltaModeChanged
//---------------------------------------------------------

void PianoRoll::deltaModeChanged(bool delta_on)
{
      if(deltaMode == delta_on)
        return;
      deltaMode = delta_on;
      
      if(canvas->selectionSize() > 0)
      {
        if(deltaMode)
          info->setValues(tickOffset, lenOffset, pitchOffset, veloOnOffset, veloOffOffset);
        else
          info->setValues(tickValue, lenValue, pitchValue, veloOnValue, veloOffValue);
      }
}

//---------------------------------------------------------
//    edit currently selected Event
//---------------------------------------------------------

void PianoRoll::noteinfoChanged(MusEGui::NoteInfo::ValType type, int val)
      {  
      int selections = canvas->selectionSize();

      if (selections == 0) {
            printf("noteinfoChanged while nothing selected\n");
            }
      else if (selections > 0) {
            if(deltaMode) {
                  // treat noteinfo values as offsets to event values
                  int delta = 0;
                  switch (type) {
                        case MusEGui::NoteInfo::VAL_TIME:
                              delta = val - tickOffset;
                              tickOffset = val;
                              break;
                        case MusEGui::NoteInfo::VAL_LEN:
                              delta = val - lenOffset;
                              lenOffset = val;
                              break;
                        case MusEGui::NoteInfo::VAL_VELON:
                              delta = val - veloOnOffset;
                              veloOnOffset = val;
                              break;
                        case MusEGui::NoteInfo::VAL_VELOFF:
                              delta = val - veloOffOffset;
                              veloOffOffset = val;
                              break;
                        case MusEGui::NoteInfo::VAL_PITCH:
                              delta = val - pitchOffset;
                              pitchOffset = val;
                              break;
                        }
                  if (delta)
                        canvas->modifySelected(type, delta);
                  }
            else {
                      switch (type) {
                            case MusEGui::NoteInfo::VAL_TIME:
                                  tickValue = val;
                                  break;
                            case MusEGui::NoteInfo::VAL_LEN:
                                  lenValue = val;
                                  break;
                            case MusEGui::NoteInfo::VAL_VELON:
                                  veloOnValue = val;
                                  break;
                            case MusEGui::NoteInfo::VAL_VELOFF:
                                  veloOffValue = val;
                                  break;
                            case MusEGui::NoteInfo::VAL_PITCH:
                                  pitchValue = val;
                                  break;
                            }
                  canvas->modifySelected(type, val, false);  // No delta mode.
                  }
            }
      }

//---------------------------------------------------------
//   addCtrl
//---------------------------------------------------------

CtrlEdit* PianoRoll::addCtrl()
      {
      CtrlEdit* ctrlEdit = new CtrlEdit(ctrlLane/* formerly splitter*/, this, xscale, false, "pianoCtrlEdit");  // ccharrett
      connect(tools2,   SIGNAL(toolChanged(int)),   ctrlEdit, SLOT(setTool(int)));
      connect(hscroll,  SIGNAL(scrollChanged(int)), ctrlEdit, SLOT(setXPos(int)));
      connect(hscroll,  SIGNAL(scaleChanged(int)),  ctrlEdit, SLOT(setXMag(int)));
      connect(ctrlEdit, SIGNAL(timeChanged(unsigned)),   SLOT(setTime(unsigned)));
      connect(ctrlEdit, SIGNAL(destroyedCtrl(CtrlEdit*)), SLOT(removeCtrl(CtrlEdit*)));
      connect(ctrlEdit, SIGNAL(yposChanged(int)), toolbar, SLOT(setInt(int)));

      ctrlEdit->setTool(tools2->curTool());
      ctrlEdit->setXPos(hscroll->pos());
      ctrlEdit->setXMag(hscroll->getScaleValue());

      ctrlEdit->show();
      ctrlEditList.push_back(ctrlEdit);
      return ctrlEdit;
      }

//---------------------------------------------------------
//   removeCtrl
//---------------------------------------------------------

void PianoRoll::removeCtrl(CtrlEdit* ctrl)
      {
      for (std::list<CtrlEdit*>::iterator i = ctrlEditList.begin();
         i != ctrlEditList.end(); ++i) {
            if (*i == ctrl) {
                  ctrlEditList.erase(i);
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   closeEvent
//   Save state. 
//   Disconnect signals which may cause crash due to Qt deferred deletion on close.
//---------------------------------------------------------

void PianoRoll::closeEvent(QCloseEvent* e)
      {
      _isDeleting = true;  // Set flag so certain signals like songChanged, which may cause crash during delete, can be ignored.

      QSettings settings("MusE", "MusE-qt");
      settings.setValue("Pianoroll/windowState", saveState());

      emit isDeleting(static_cast<TopWin*>(this));
      e->accept();
      }

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void PianoRoll::readConfiguration(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
                  break;
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::TagStart:
                        if (tag == "raster")
                              _rasterInit = xml.parseInt();
                        else if (tag == "colormode")
                              colorModeInit = xml.parseInt();
                        else if (tag == "topwin")
                              TopWin::readConfiguration(PIANO_ROLL,xml);
                        else
                              xml.unknown("PianoRoll");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "pianoroll")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeConfiguration
//---------------------------------------------------------

void PianoRoll::writeConfiguration(int level, MusECore::Xml& xml)
      {
      xml.tag(level++, "pianoroll");
      xml.intTag(level, "raster", _rasterInit);
      xml.intTag(level, "colormode", colorModeInit);
      TopWin::writeConfiguration(PIANO_ROLL, level, xml);
      xml.etag(level, "pianoroll");
      }

//---------------------------------------------------------
//   soloChanged
//    signal from solo button
//---------------------------------------------------------

void PianoRoll::soloChanged(bool flag)
      {
      MusEGlobal::audio->msgSetSolo(canvas->track(), flag);
      MusEGlobal::song->update(SC_SOLO);
      }

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void PianoRoll::setRaster(int val)
      {
      _rasterInit = val;
      MidiEditor::setRaster(val);
      canvas->redrawGrid();
      focusCanvas();     // give back focus after kb input
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void PianoRoll::writeStatus(int level, MusECore::Xml& xml) const
      {
      writePartList(level, xml);
      xml.tag(level++, "pianoroll");
      MidiEditor::writeStatus(level, xml);
      splitter->writeStatus(level, xml);
      hsplitter->writeStatus(level, xml);  
      
      for (std::list<CtrlEdit*>::const_iterator i = ctrlEditList.begin();
         i != ctrlEditList.end(); ++i) {
            (*i)->writeStatus(level, xml);
            }

      xml.intTag(level, "steprec", canvas->steprec());
      xml.intTag(level, "midiin", canvas->midiin());
      xml.intTag(level, "tool", int(canvas->tool()));
      xml.intTag(level, "playEvents", _playEvents);
      xml.intTag(level, "xpos", hscroll->pos());
      xml.intTag(level, "xmag", hscroll->mag());
      xml.intTag(level, "ypos", vscroll->pos());
      xml.intTag(level, "ymag", vscroll->mag());
      xml.tag(level, "/pianoroll");
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void PianoRoll::readStatus(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
                  break;
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::TagStart:
                        if (tag == "steprec") {
                              int val = xml.parseInt();
                              canvas->setSteprec(val);
                              srec->setChecked(val);
                              }
                        else if (tag == "midiin") {
                              int val = xml.parseInt();
                              canvas->setMidiin(val);
                              midiin->setChecked(val);
                              }
                        else if (tag == "tool") {
                              int tool = xml.parseInt();
                              canvas->setTool(tool);
                              tools2->set(tool);
                              }
                        else if (tag == "midieditor")
                              MidiEditor::readStatus(xml);
                        else if (tag == "ctrledit") {
                              CtrlEdit* ctrl = addCtrl();
                              ctrl->readStatus(xml);
                              }
                        else if (tag == splitter->objectName())
                              splitter->readStatus(xml);
                        else if (tag == hsplitter->objectName())
                              hsplitter->readStatus(xml);
                        else if (tag == "playEvents") {
                              _playEvents = xml.parseInt();
                              canvas->playEvents(_playEvents);
                              speaker->setChecked(_playEvents);
                              }
                        else if (tag == "xmag")
                              hscroll->setMag(xml.parseInt());
                        else if (tag == "xpos")
                              hscroll->setPos(xml.parseInt());
                        else if (tag == "ymag")
                              vscroll->setMag(xml.parseInt());
                        else if (tag == "ypos")
                              vscroll->setPos(xml.parseInt());
                        else
                              xml.unknown("PianoRoll");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "pianoroll") {
                              _rasterInit = _raster;
                              toolbar->setRaster(_raster);
                              canvas->redrawGrid();
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

static int rasterTable[] = {
      //-9----8-  7    6     5     4    3(1/4)     2   1
      4,  8, 16, 32,  64, 128, 256,  512, 1024,  // triple
      6, 12, 24, 48,  96, 192, 384,  768, 1536,
      9, 18, 36, 72, 144, 288, 576, 1152, 2304   // dot
      };

//---------------------------------------------------------
//   viewKeyPressEvent
//---------------------------------------------------------

void PianoRoll::keyPressEvent(QKeyEvent* event)
      {
      if (info->hasFocus()) {
            event->ignore();
            return;
            }

      int index;
      int n = sizeof(rasterTable)/sizeof(*rasterTable);
      for (index = 0; index < n; ++index)
            if (rasterTable[index] == raster())
                  break;
      if (index == n) {
            index = 0;
            // raster 1 is not in table
            }
      int off = (index / 9) * 9;
      index   = index % 9;

      int val = 0;

      PianoCanvas* pc = (PianoCanvas*)canvas;
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
      else if (key == shortcuts[SHRT_TOOL_LINEDRAW].key) {
            tools2->set(MusEGui::DrawTool);
            return;
            }
      else if (key == shortcuts[SHRT_POS_INC].key) {
            pc->pianoCmd(CMD_RIGHT);
            return;
            }
      else if (key == shortcuts[SHRT_POS_DEC].key) {
            pc->pianoCmd(CMD_LEFT);
            return;
            }
      else if (key == shortcuts[SHRT_POS_INC_NOSNAP].key) {
            pc->pianoCmd(CMD_RIGHT_NOSNAP);
            return;
            }
      else if (key == shortcuts[SHRT_POS_DEC_NOSNAP].key) {
            pc->pianoCmd(CMD_LEFT_NOSNAP);
            return;
            }
      else if (key == shortcuts[SHRT_INSERT_AT_LOCATION].key) {
            pc->pianoCmd(CMD_INSERT);
            return;
            }
      else if (key == Qt::Key_Backspace) {
            pc->pianoCmd(CMD_BACKSPACE);
            return;
            }
      else if (key == shortcuts[SHRT_ZOOM_IN].key) {
            int mag = hscroll->mag();
            int zoomlvl = MusEGui::ScrollScale::getQuickZoomLevel(mag);
            if (zoomlvl < 23)
                  zoomlvl++;

            int newmag = MusEGui::ScrollScale::convertQuickZoomLevelToMag(zoomlvl);
            hscroll->setMag(newmag);
            //printf("mag = %d zoomlvl = %d newmag = %d\n", mag, zoomlvl, newmag);
            return;
            }
      else if (key == shortcuts[SHRT_ZOOM_OUT].key) {
            int mag = hscroll->mag();
            int zoomlvl = MusEGui::ScrollScale::getQuickZoomLevel(mag);
            if (zoomlvl > 1)
                  zoomlvl--;

            int newmag = MusEGui::ScrollScale::convertQuickZoomLevelToMag(zoomlvl);
            hscroll->setMag(newmag);
            //printf("mag = %d zoomlvl = %d newmag = %d\n", mag, zoomlvl, newmag);
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
      else if (key == shortcuts[SHRT_SET_QUANT_1].key)
            val = rasterTable[8 + off];
      else if (key == shortcuts[SHRT_SET_QUANT_2].key)
            val = rasterTable[7 + off];
      else if (key == shortcuts[SHRT_SET_QUANT_3].key)
            val = rasterTable[6 + off];
      else if (key == shortcuts[SHRT_SET_QUANT_4].key)
            val = rasterTable[5 + off];
      else if (key == shortcuts[SHRT_SET_QUANT_5].key)
            val = rasterTable[4 + off];
      else if (key == shortcuts[SHRT_SET_QUANT_6].key)
            val = rasterTable[3 + off];
      else if (key == shortcuts[SHRT_SET_QUANT_7].key)
            val = rasterTable[2 + off];
      else if (key == shortcuts[SHRT_TOGGLE_TRIOL].key)
            val = rasterTable[index + ((off == 0) ? 9 : 0)];
      else if (key == shortcuts[SHRT_EVENT_COLOR].key) {
            if (colorMode == 0)
                  colorMode = 1;
            else if (colorMode == 1)
                  colorMode = 2;
            else
                  colorMode = 0;
            setEventColorMode(colorMode);
            return;
            }
      else if (key == shortcuts[SHRT_TOGGLE_PUNCT].key)
            val = rasterTable[index + ((off == 18) ? 9 : 18)];

      else if (key == shortcuts[SHRT_TOGGLE_PUNCT2].key) {//CDW
            if ((off == 18) && (index > 2)) {
                  val = rasterTable[index + 9 - 1];
                  }
            else if ((off == 9) && (index < 8)) {
                  val = rasterTable[index + 18 + 1];
                  }
            else
                  return;
            }
      else { //Default:
            event->ignore();
            return;
            }
      setRaster(val);
      toolbar->setRaster(_raster);
      }

//---------------------------------------------------------
//   setSteprec
//---------------------------------------------------------

void PianoRoll::setSteprec(bool flag)
      {
      canvas->setSteprec(flag);
      if (flag == false)
            midiin->setChecked(flag);
      }

//---------------------------------------------------------
//   eventColorModeChanged
//---------------------------------------------------------

void PianoRoll::eventColorModeChanged(int mode)
      {
      colorMode = mode;
      colorModeInit = colorMode;
      
      ((PianoCanvas*)(canvas))->setColorMode(colorMode);
      }

//---------------------------------------------------------
//   setEventColorMode
//---------------------------------------------------------

void PianoRoll::setEventColorMode(int mode)
      {
      colorMode = mode;
      colorModeInit = colorMode;
      
      evColorBlueAction->setChecked(mode == 0);
      evColorPitchAction->setChecked(mode == 1);
      evColorVelAction->setChecked(mode == 2);
      
      ((PianoCanvas*)(canvas))->setColorMode(colorMode);
      }

//---------------------------------------------------------
//   clipboardChanged
//---------------------------------------------------------

void PianoRoll::clipboardChanged()
      {
      editPasteAction->setEnabled(QApplication::clipboard()->mimeData()->hasFormat(QString("text/x-muse-groupedeventlists")));
      editPasteDialogAction->setEnabled(QApplication::clipboard()->mimeData()->hasFormat(QString("text/x-muse-groupedeventlists")));
      }

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void PianoRoll::selectionChanged()
      {
      bool flag = canvas->selectionSize() > 0;
      editCutAction->setEnabled(flag);
      editCopyAction->setEnabled(flag);
      editDelEventsAction->setEnabled(flag);
      }

//---------------------------------------------------------
//   setSpeaker
//---------------------------------------------------------

void PianoRoll::setSpeaker(bool val)
      {
      _playEvents = val;
      canvas->playEvents(_playEvents);
      }

//---------------------------------------------------------
//   initShortcuts
//---------------------------------------------------------

void PianoRoll::initShortcuts()
      {
      editCutAction->setShortcut(shortcuts[SHRT_CUT].key);
      editCopyAction->setShortcut(shortcuts[SHRT_COPY].key);
      editCopyRangeAction->setShortcut(shortcuts[SHRT_COPY_RANGE].key);
      editPasteAction->setShortcut(shortcuts[SHRT_PASTE].key);
      editPasteDialogAction->setShortcut(shortcuts[SHRT_PASTE_DIALOG].key);
      editDelEventsAction->setShortcut(shortcuts[SHRT_DELETE].key);
      
      selectAllAction->setShortcut(shortcuts[SHRT_SELECT_ALL].key); 
      selectNoneAction->setShortcut(shortcuts[SHRT_SELECT_NONE].key);
      selectInvertAction->setShortcut(shortcuts[SHRT_SELECT_INVERT].key);
      selectInsideLoopAction->setShortcut(shortcuts[SHRT_SELECT_ILOOP].key);
      selectOutsideLoopAction->setShortcut(shortcuts[SHRT_SELECT_OLOOP].key);
      selectPrevPartAction->setShortcut(shortcuts[SHRT_SELECT_PREV_PART].key);
      selectNextPartAction->setShortcut(shortcuts[SHRT_SELECT_NEXT_PART].key);
      
      eventColor->menuAction()->setShortcut(shortcuts[SHRT_EVENT_COLOR].key);
      //evColorBlueAction->setShortcut(shortcuts[  ].key);
      //evColorPitchAction->setShortcut(shortcuts[  ].key);
      //evColorVelAction->setShortcut(shortcuts[  ].key);
      
      funcQuantizeAction->setShortcut(shortcuts[SHRT_QUANTIZE].key);
      
      funcGateTimeAction->setShortcut(shortcuts[SHRT_MODIFY_GATE_TIME].key);
      funcModVelAction->setShortcut(shortcuts[SHRT_MODIFY_VELOCITY].key);
      funcTransposeAction->setShortcut(shortcuts[SHRT_TRANSPOSE].key);
      funcEraseEventAction->setShortcut(shortcuts[SHRT_ERASE_EVENT].key);
      funcNoteShiftAction->setShortcut(shortcuts[SHRT_NOTE_SHIFT].key);
      funcSetFixedLenAction->setShortcut(shortcuts[SHRT_FIXED_LEN].key);
      funcDelOverlapsAction->setShortcut(shortcuts[SHRT_DELETE_OVERLAPS].key);
      
      }

//---------------------------------------------------------
//   execDeliveredScript
//---------------------------------------------------------
void PianoRoll::execDeliveredScript(int id)
{
      QString scriptfile = MusEGlobal::song->getScriptPath(id, true);
      MusEGlobal::song->executeScript(scriptfile.toAscii().data(), parts(), raster(), true);
}

//---------------------------------------------------------
//   execUserScript
//---------------------------------------------------------
void PianoRoll::execUserScript(int id)
{
      QString scriptfile = MusEGlobal::song->getScriptPath(id, false);
      MusEGlobal::song->executeScript(scriptfile.toAscii().data(), parts(), raster(), true);
}

//---------------------------------------------------------
//   newCanvasWidth
//---------------------------------------------------------

void PianoRoll::newCanvasWidth(int /*w*/)
      {
/*       DELETETHIS whole function?
      int nw = w + (vscroll->width() - 18); // 18 is the fixed width of the CtlEdit VScale widget.
      if(nw < 1)
        nw = 1;
        
      for (std::list<CtrlEdit*>::iterator i = ctrlEditList.begin();
         i != ctrlEditList.end(); ++i) {
            // Changed by Tim. p3.3.7
            //(*i)->setCanvasWidth(w);
            (*i)->setCanvasWidth(nw);
            }
            
      updateHScrollRange();
*/      
      }

//---------------------------------------------------------
//   toggleTrackInfo
//---------------------------------------------------------

void PianoRoll::toggleTrackInfo()
{
  bool vis = midiTrackInfo->isVisible();
  infoScroll->setVisible(!vis);
  infoScroll->setEnabled(!vis);
}

} // namespace MusEGui
