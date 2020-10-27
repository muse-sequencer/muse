//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pianoroll.cpp,v 1.25.2.15 2009/11/16 11:29:33 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012-2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include <QSplitter>
#include <QSizeGrip>
#include <QLabel>
#include <QToolTip>
#include <QMenuBar>
#include <QWhatsThis>
#include <QApplication>
#include <QClipboard>
#include <QGridLayout>
#include <QMimeData>
#include <QScrollArea>
#include <QSettings>
#include <QCursor>
#include <QRect>
#include <QPixmap>

#include <stdio.h>

#include "pianoroll.h"
#include "mtscale.h"
#include "prcanvas.h"
#include "scrollbar.h"
#include "utils.h"
#include "globals.h"
#include "app.h"
#include "song.h"
#include "midiport.h"
#include "gconfig.h"
#include "icons.h"
#include "audio.h"
#include "functions.h"
#include "helper.h"
#include "menutitleitem.h"
#include "operations.h"

#include "cmd.h"
#include "shortcuts.h"

#include "trackinfo_layout.h"
#include "midi_editor_layout.h"

// Forwards from header:
#include <QAction>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>
#include <QPoint>
#include <QCloseEvent>
#include <QKeyEvent>
#include "cobject.h"
#include "event.h"
#include "xml.h"
#include "ctrl/ctrledit.h"
#include "pitchlabel.h"
#include "scrollscale.h"
#include "splitter.h"
#include "tb1.h"
#include "piano.h"
#include "popupmenu.h"
#include "tools.h"

namespace MusEGui {

int PianoRoll::_rasterInit = 96;
int PianoRoll::_trackInfoWidthInit = 50;
int PianoRoll::_canvasWidthInit = 300;
MidiEventColorMode PianoRoll::colorModeInit = MidiEventColorMode::blueEvents;

// Initial zoom levels:
static const int xscale = -10;
static const int yscale = 2;

static int pianorollTools = MusEGui::PointerTool | MusEGui::PencilTool | MusEGui::RubberTool | MusEGui::DrawTool | PanTool | ZoomTool;


//---------------------------------------------------------
//   PianoRoll
//---------------------------------------------------------

PianoRoll::PianoRoll(MusECore::PartList* pl, QWidget* parent, const char* name, unsigned initPos, bool showDefaultControls)
   : MidiEditor(TopWin::PIANO_ROLL, _rasterInit, pl, parent, name)
      {
      deltaMode     = false;
      tickValue     = 0;
      lenValue      = 0;
      pitchValue    = 0;
      veloOnValue   = 1;
      veloOffValue  = 0;
      firstValueSet = false;
      tickOffset    = 0;
      lenOffset     = 0;
      pitchOffset   = 0;
      veloOnOffset  = 0;
      veloOffOffset = 0;
      lastSelections = 0;
      _playEvents    = true;
      _playEventsMode = EventCanvas::PlayEventsSingleNote;
      colorMode      = colorModeInit;
      _canvasXOrigin = DefaultCanvasXOrigin;
      _minXMag = -25;
      _maxXMag = 2;
      
      // Request to set the raster, but be sure to use the one it chooses,
      //  which may be different than the one requested.
      _rasterInit = _rasterizerModel->checkRaster(_rasterInit);
      _raster = _rasterInit;

      _pianoWidth = 40;
      ensurePolished();
      if (_pianoWidth < 40)
          _pianoWidth = 40;

      const MusECore::PartList* part_list = parts();
      // Default initial pianoroll view state.
      _viewState = MusECore::MidiPartViewState (0, KH * 30, xscale, yscale);
      // Include a velocity controller in the default initial view state.
      _viewState.addController(MusECore::MidiCtrlViewState(MusECore::CTRL_VELOCITY));
      if(part_list && !part_list->empty())
      {
        // If the parts' view states have never been initialized before,
        //  do it now with the desired pianoroll initial state.
        for(MusECore::ciPart i = part_list->begin(); i != part_list->end(); ++i)
        {
          if(!i->second->viewState().isValid())
            i->second->setViewState(_viewState);
        }
        // Now take our initial view state from the first part found in the list.
        // Don't bother if not showing default controls, since something else
        //  will likely take care of it, like the song file loading routines.
        if(showDefaultControls)
          _viewState = part_list->begin()->second->viewState();
      }
      
      //---------Menu----------------------------------
      
      menuEdit = menuBar()->addMenu(tr("&Edit"));      
      
      menuEdit->addActions(MusEGlobal::undoRedo->actions());
      
      menuEdit->addSeparator();
      
      editCutAction = menuEdit->addAction(QIcon(*editcutIconSet), tr("C&ut"));
      connect(editCutAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_CUT); } );
      
      editCopyAction = menuEdit->addAction(QIcon(*editcopyIconSet), tr("&Copy"));
      connect(editCopyAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_COPY); } );
      
      editCopyRangeAction = menuEdit->addAction(QIcon(*editcopyIconSet), tr("Copy events in range"));
      connect(editCopyRangeAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_COPY_RANGE); } );
      
      editPasteAction = menuEdit->addAction(QIcon(*editpasteIconSet), tr("&Paste"));
      connect(editPasteAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_PASTE); } );
      
      editPasteToCurPartAction = menuEdit->addAction(QIcon(*editpasteIconSet), tr("Paste to current part"));
      connect(editPasteToCurPartAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_PASTE_TO_CUR_PART); } );

      editPasteDialogAction = menuEdit->addAction(QIcon(*editpasteIconSet), tr("Paste (with dialog)"));
      connect(editPasteDialogAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_PASTE_DIALOG); } );
      
      menuEdit->addSeparator();
      
      editDelEventsAction = menuEdit->addAction(tr("Delete &Events"));
      connect(editDelEventsAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_DEL); } );
      
      menuEdit->addSeparator();

      menuSelect = menuEdit->addMenu(QIcon(*selectIcon), tr("&Select"));

      selectAllAction = menuSelect->addAction(QIcon(*select_allIcon), tr("Select &All"));
      connect(selectAllAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_SELECT_ALL); } );
      
      selectNoneAction = menuSelect->addAction(QIcon(*select_deselect_allIcon), tr("&Deselect All"));
      connect(selectNoneAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_SELECT_NONE); } );
      
      selectInvertAction = menuSelect->addAction(QIcon(*select_invert_selectionIcon), tr("Invert &Selection"));
      connect(selectInvertAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_SELECT_INVERT); } );
      
      menuSelect->addSeparator();
      
      selectInsideLoopAction = menuSelect->addAction(QIcon(*select_inside_loopIcon), tr("&Inside Loop"));
      connect(selectInsideLoopAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_SELECT_ILOOP); } );
      
      selectOutsideLoopAction = menuSelect->addAction(QIcon(*select_outside_loopIcon), tr("&Outside Loop"));
      connect(selectOutsideLoopAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_SELECT_OLOOP); } );
      
      menuSelect->addSeparator();
      
      selectPrevPartAction = menuSelect->addAction(QIcon(*select_all_parts_on_trackIcon), tr("&Previous Part"));
      connect(selectPrevPartAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_SELECT_PREV_PART); } );
      
      selectNextPartAction = menuSelect->addAction(QIcon(*select_all_parts_on_trackIcon), tr("&Next Part"));
      connect(selectNextPartAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_SELECT_NEXT_PART); } );

      menuEdit->addSeparator();
      startListEditAction = menuEdit->addAction(*listeditSVGIcon, tr("Event List..."));
      connect(startListEditAction, SIGNAL(triggered()), MusEGlobal::muse, SLOT(startListEditor()));

      menuFunctions = menuBar()->addMenu(tr("Fu&nctions"));

//      menuFunctions->setTearOffEnabled(true);
      
      funcQuantizeAction = menuFunctions->addAction(tr("Quantize"));
      connect(funcQuantizeAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_QUANTIZE); } );
      
      funcGateTimeAction = menuFunctions->addAction(tr("Modify Note Length"));
      connect(funcGateTimeAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_MODIFY_GATE_TIME); } );
      
      funcModVelAction = menuFunctions->addAction(tr("Modify Velocity"));
      connect(funcModVelAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_MODIFY_VELOCITY); } );
      
      funcCrescAction = menuFunctions->addAction(tr("Crescendo/Decrescendo"));
      connect(funcCrescAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_CRESCENDO); } );
      
      funcTransposeAction = menuFunctions->addAction(tr("Transpose"));
      connect(funcTransposeAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_TRANSPOSE); } );
            
      funcEraseEventAction = menuFunctions->addAction(tr("Erase Events"));
      connect(funcEraseEventAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_ERASE_EVENT); } );
      
      funcNoteShiftAction = menuFunctions->addAction(tr("Move Notes"));
      connect(funcNoteShiftAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_NOTE_SHIFT); } );
            
      funcSetFixedLenAction = menuFunctions->addAction(tr("Set Fixed Length"));
      connect(funcSetFixedLenAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_FIXED_LEN); } );
      
      funcDelOverlapsAction = menuFunctions->addAction(tr("Delete Overlaps"));
      connect(funcDelOverlapsAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_DELETE_OVERLAPS); } );

      QAction* funcLegatoAction = menuFunctions->addAction(tr("Legato"));
      connect(funcLegatoAction, &QAction::triggered, [this]() { cmd(PianoCanvas::CMD_LEGATO); } );
            

      //----------------------
      // Scripts:
      //----------------------

      menuPlugins = menuBar()->addMenu(tr("&Scripts"));
      connect(&_scriptReceiver,
              &MusECore::ScriptReceiver::execDeliveredScriptReceived,
              [this](int id) { execDeliveredScript(id); } );
      connect(&_scriptReceiver,
              &MusECore::ScriptReceiver::execUserScriptReceived,
              [this](int id) { execUserScript(id); } );
      MusEGlobal::song->populateScriptMenu(menuPlugins, &_scriptReceiver);

      menuConfig = menuBar()->addMenu(tr("&Display"));
      menuConfig->menuAction()->setStatusTip(tr("Display menu: View-specific display options."));

      menuConfig->addAction(subwinAction);
//      menuConfig->addAction(shareAction);
      menuConfig->addAction(fullscreenAction);

      menuConfig->addSeparator();

      eventColor = menuConfig->addMenu(tr("&Event Color"));
      
      QActionGroup* actgrp = new QActionGroup(this);
      actgrp->setExclusive(true);
      
      evColorBlueAction = actgrp->addAction(tr("&Blue"));
      evColorBlueAction->setCheckable(true);
      connect(evColorBlueAction, &QAction::triggered, [this]() { eventColorModeChanged(MidiEventColorMode::blueEvents); } );
      
      evColorPitchAction = actgrp->addAction(tr("&Pitch colors"));
      evColorPitchAction->setCheckable(true);
      connect(evColorPitchAction, &QAction::triggered, [this]() { eventColorModeChanged(MidiEventColorMode::pitchColorEvents); } );
      
      evColorVelAction = actgrp->addAction(tr("&Velocity colors"));
      evColorVelAction->setCheckable(true);
      connect(evColorVelAction, &QAction::triggered, [this]() { eventColorModeChanged(MidiEventColorMode::velocityColorEvents); } );
      
      eventColor->addActions(actgrp->actions());
      
//      menuConfig->addSeparator();
      addControllerMenu = new PopupMenu(tr("Add controller view"), this, true);
      addControllerMenu->setIcon(*midiControllerNewSVGIcon);
      menuConfig->addMenu(addControllerMenu);
      connect(addControllerMenu, &QMenu::aboutToShow, [this]() { ctrlMenuAboutToShow(); } );
      connect(addControllerMenu, &QMenu::aboutToHide, [this]() { ctrlMenuAboutToHide(); } );
      connect(addControllerMenu, &QMenu::triggered, [this](QAction* act) { ctrlPopupTriggered(act); } );
      
      //---------ToolBar----------------------------------

      // NOTICE: Please ensure that any tool bar object names here match the names assigned 
      //          to identical or similar toolbars in class MusE or other TopWin classes. 
      //         This allows MusE::setCurrentMenuSharingTopwin() to do some magic
      //          to retain the original toolbar layout. If it finds an existing
      //          toolbar with the same object name, it /replaces/ it using insertToolBar(),
      //          instead of /appending/ with addToolBar().

      addToolBarBreak();
      
      // Already has an object name.
      tools2 = new MusEGui::EditToolBar(this, pianorollTools);
      addToolBar(tools2);

      tools = addToolBar(tr("Pianoroll tools"));
      tools->setObjectName("Pianoroll tools");

      addctrl = new QToolButton();
      addctrl->setToolTip(tr("Add controller view"));
      addctrl->setIcon(*midiControllerNewSVGIcon);
      addctrl->setFocusPolicy(Qt::NoFocus);
      connect(addctrl, &QToolButton::pressed, [this]() { addCtrlClicked(); } );
      tools->addWidget(addctrl);

      srec  = new QToolButton();
      srec->setToolTip(tr("Step record"));
      srec->setIcon(*steprecSVGIcon);
      srec->setCheckable(true);
      srec->setFocusPolicy(Qt::NoFocus);
      tools->addWidget(srec);

      midiin  = new QToolButton();
      midiin->setToolTip(tr("Midi input"));
      midiin->setIcon(*midiinSVGIcon);
      midiin->setCheckable(true);
      midiin->setFocusPolicy(Qt::NoFocus);
      tools->addWidget(midiin);

      speaker  = new QToolButton();
      speaker->setToolTip(tr("Play events"));
      speaker->setIcon(*speakerSingleNoteSVGIcon);
      speaker->setCheckable(true);
      speaker->setChecked(true);
      speaker->setFocusPolicy(Qt::NoFocus);
      
      QMenu* speakerPopupMenu = new QMenu(this);
      speaker->setMenu(speakerPopupMenu);
      speaker->setPopupMode(QToolButton::MenuButtonPopup);
      speakerSingleNote = new QAction(*speakerSingleNoteSVGIcon, tr("Play single note"), this);
      speakerChords = new QAction(*speakerChordsSVGIcon, tr("Play chords"), this);
      speakerPopupMenu->addAction(speakerSingleNote);
      speakerPopupMenu->addAction(speakerChords);
      connect(speakerSingleNote, &QAction::triggered, [this](bool checked) { setSpeakerSingleNoteMode(checked); } );
      connect(speakerChords, &QAction::triggered, [this](bool checked) { setSpeakerChordMode(checked); } );

      tools->addWidget(speaker);

//      QAction* whatsthis = QWhatsThis::createAction(this);
//      whatsthis->setIcon(*whatsthisSVGIcon);
//      tools->addAction(whatsthis);
      
      toolbar = new MusEGui::Toolbar1(_rasterizerModel, this, _rasterInit);
      toolbar->setObjectName("Pianoroll Pos/Snap/Solo-tools");
      addToolBar(toolbar);

      addToolBarBreak();
      
      info    = new MusEGui::NoteInfo(this);
      info->setObjectName("Pianoroll Note Info");
      addToolBar(info);

      //---------------------------------------------------
      //    split
      //---------------------------------------------------

      splitter = new MusEGui::Splitter(Qt::Vertical, mainw, "splitter");
      //splitter->setHandleWidth(2);
      
      hsplitter = new MusEGui::Splitter(Qt::Horizontal, mainw, "hsplitter");
      hsplitter->setChildrenCollapsible(true);
      //hsplitter->setHandleWidth(4);

      hscroll = new MusEGui::ScrollScale(
        (_minXMag * MusEGlobal::config.division) / 384,
        _maxXMag,
        _viewState.xscale(),
        20000,
        Qt::Horizontal,
        mainw);

      QSizeGrip* corner = new QSizeGrip(mainw);

      trackInfoWidget = new TrackInfoWidget(hsplitter);
      genTrackInfo(trackInfoWidget);
      
      QWidget* gridS2_w = new QWidget();
      gridS2_w->setObjectName("gridS2_w");
      gridS2_w->setContentsMargins(0, 0, 0, 0);
      QGridLayout* gridS2 = new QGridLayout(gridS2_w);
      gridS2->setContentsMargins(0, 0, 0, 0);
      gridS2->setSpacing(0);  
      gridS2->setRowStretch(0, 100);
      gridS2->setColumnStretch(1, 100);
      gridS2->addItem(new QSpacerItem(_pianoWidth, 0),    0, 0);
      gridS2->addWidget(hscroll, 0, 1);
      gridS2->addWidget(corner,  0, 2, Qt::AlignBottom|Qt::AlignRight);
      gridS2_w->setMaximumHeight(hscroll->sizeHint().height());
      gridS2_w->setMinimumHeight(hscroll->sizeHint().height());
      
      QWidget* splitter_w = new QWidget();
      splitter_w->setObjectName("splitter_w");
      splitter_w->setContentsMargins(0, 0, 0, 0);
      QVBoxLayout* splitter_vbox = new QVBoxLayout(splitter_w);
      splitter_vbox->setContentsMargins(0, 0, 0, 0);
      splitter_vbox->setSpacing(0);  
      splitter_vbox->addWidget(splitter);
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
      splitter->setSizePolicy(epolicy);

      mainGrid->addWidget(hsplitter, 0, 0, 1, 1);

      QList<int> mops;
      mops.append(_trackInfoWidthInit);
      mops.append(_canvasWidthInit);
      hsplitter->setSizes(mops);

      QWidget* split1     = new QWidget(splitter);
      split1->setObjectName("split1");
      QGridLayout* gridS1 = new QGridLayout(split1);
      gridS1->setContentsMargins(0, 0, 0, 0);
      gridS1->setSpacing(0);  

      time                = new MusEGui::MTScale(_raster, split1, _viewState.xscale());
      piano               = new Piano(split1, _viewState.yscale(), _pianoWidth, this);
      canvas              = new PianoCanvas(this, split1, _viewState.xscale(), _viewState.yscale());
      vscroll             = new MusEGui::ScrollScale(-2, 6, _viewState.yscale(), KH * 75, Qt::Vertical, split1);
      setCurDrumInstrument(piano->curSelectedPitch());

      canvas->setOrigin(_canvasXOrigin, 0);
      canvas->setCanvasTools(pianorollTools);
      canvas->setFocus();
      connect(canvas, SIGNAL(toolChanged(int)), tools2, SLOT(set(int)));
      connect(canvas, SIGNAL(horizontalZoom(bool, const QPoint&)), SLOT(horizontalZoom(bool, const QPoint&)));
      connect(canvas, SIGNAL(horizontalZoom(int, const QPoint&)), SLOT(horizontalZoom(int, const QPoint&)));
      connect(canvas, SIGNAL(curPartHasChanged(MusECore::Part*)), SLOT(updateTrackInfo()));
      time->setOrigin(_canvasXOrigin, 0);

      gridS1->setRowStretch(2, 100);
      gridS1->setColumnStretch(1, 100);     

      gridS1->addWidget(time,                   0, 1, 1, 2);
      gridS1->addWidget(MusECore::hLine(split1),          1, 0, 1, 3);
      gridS1->addWidget(piano,                  2,    0);
      gridS1->addWidget(canvas,                 2,    1);
      gridS1->addWidget(vscroll,                2,    2);

      piano->setFixedWidth(_pianoWidth);

      connect(tools2, SIGNAL(toolChanged(int)), canvas,   SLOT(setTool(int)));

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
      connect(piano, SIGNAL(redirectWheelEvent(QWheelEvent*)), canvas, SLOT(redirectedWheelEvent(QWheelEvent*)));
      connect(piano, SIGNAL(wheelStep(bool)), vscroll, SLOT(stepScale(bool)));
      connect(srec, SIGNAL(toggled(bool)), SLOT(setSteprec(bool)));
      connect(midiin, SIGNAL(toggled(bool)), canvas, SLOT(setMidiin(bool)));
      connect(speaker, SIGNAL(toggled(bool)), SLOT(setSpeaker(bool)));
      connect(canvas, SIGNAL(followEvent(int)), SLOT(follow(int)));

      connect(info, SIGNAL(returnPressed()),          SLOT(focusCanvas()));
      connect(info, SIGNAL(escapePressed()),          SLOT(focusCanvas()));

      connect(hscroll, SIGNAL(scaleChanged(int)),  SLOT(updateHScrollRange()));

      piano->setYPos(_viewState.yscroll());
      canvas->setYPos(_viewState.yscroll());
      vscroll->setOffset(_viewState.yscroll());

      info->setEnabled(false);

      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged1(MusECore::SongChangedStruct_t)));

      setWindowTitle(canvas->getCaption());
      
      updateHScrollRange();

      // connect to toolbar
      connect(canvas,   SIGNAL(pitchChanged(int)), toolbar, SLOT(setPitch(int)));  
      connect(canvas,   SIGNAL(timeChanged(unsigned)),  SLOT(setTime(unsigned)));
      connect(piano,    SIGNAL(pitchChanged(int)), toolbar, SLOT(setPitch(int)));
      connect(time,     SIGNAL(timeChanged(unsigned)),  SLOT(setTime(unsigned)));
      connect(toolbar, &Toolbar1::rasterChanged, [this](int raster) { setRaster(raster); } );
      connect(toolbar,  SIGNAL(soloChanged(bool)), SLOT(soloChanged(bool)));

      setFocusPolicy(Qt::NoFocus);
      
      setEventColorMode(colorMode);

      QClipboard* cb = QApplication::clipboard();
      connect(cb, SIGNAL(dataChanged()), SLOT(clipboardChanged()));

      clipboardChanged(); // enable/disable "Paste"
      selectionChanged(); // enable/disable "Copy" & "Paste"
//       initShortcuts(); // initialize shortcuts
      configChanged();  // set configuration values, initialize shortcuts

      // Don't bother if not showing default stuff, since something else
      //  will likely take care of it, like the song file loading routines.
      if(showDefaultControls)
      {
        const MusECore::Pos cpos=MusEGlobal::song->cPos();
        canvas->setPos(0, cpos.tick(), true);
        canvas->selectAtTick(cpos.tick());
          
        unsigned pos=0;
        if(initPos >= INT_MAX)
          pos = MusEGlobal::song->cpos();
        else
          pos = initPos;
        if(pos > INT_MAX)
          pos = INT_MAX;
        //if (pos)
        hscroll->setOffset((int)pos);
      }

      if(canvas->track())
      {
        updateTrackInfo();
        toolbar->setSolo(canvas->track()->solo());
      }

      setSpeakerMode(_playEventsMode);

      initTopwinState();
      finalizeInit();

      // Add initial controllers.
      // Don't bother if not showing default stuff, since something else
      //  will likely take care of it, like the song file loading routines.
      if(showDefaultControls)
      {
        CtrlEdit* ctrl_edit;
        const MusECore::MidiCtrlViewStateList& mcvsl = _viewState.controllers();
        for(MusECore::ciMidiCtrlViewState i = mcvsl.cbegin(); i != mcvsl.cend(); ++i)
        {
          const MusECore::MidiCtrlViewState& mcvs = *i;
          ctrl_edit = addCtrl(mcvs._num);
          if(ctrl_edit)
            ctrl_edit->setPerNoteVel(mcvs._perNoteVel);
        }
      }
      
      }

//---------------------------------------------------------
//   songChanged1
//---------------------------------------------------------

void PianoRoll::songChanged1(MusECore::SongChangedStruct_t bits)
      {
        if(_isDeleting)  // Ignore while while deleting to prevent crash.
          return;

        // We must catch this first and be sure to update the strips.
        if(bits & SC_TRACK_REMOVED)
        {
          checkTrackInfoTrack();
        }
        
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
          if(canvas->track())
            toolbar->setSolo(canvas->track()->solo());
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
//   configChanged
//---------------------------------------------------------

void PianoRoll::configChanged()
      {
      if (MusEGlobal::config.canvasBgPixmap.isEmpty()) {
            canvas->setBg(MusEGlobal::config.midiCanvasBg);
            canvas->setBg(QPixmap());
      }
      else {
            canvas->setBg(QPixmap(MusEGlobal::config.canvasBgPixmap));
      }
      initShortcuts();
      }

//---------------------------------------------------------
//   horizontalZoom
//---------------------------------------------------------

void PianoRoll::horizontalZoom(bool zoom_in, const QPoint& glob_pos)
{
  int mag = hscroll->mag();
  int zoomlvl = MusEGui::ScrollScale::getQuickZoomLevel(mag);
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
  int newmag = MusEGui::ScrollScale::convertQuickZoomLevelToMag(zoomlvl);

  QPoint cp = canvas->mapFromGlobal(glob_pos);
  QPoint sp = splitter->mapFromGlobal(glob_pos);
  if(cp.x() >= 0 && cp.x() < canvas->width() && sp.y() >= 0 && sp.y() < splitter->height())
    hscroll->setMag(newmag, cp.x());
}

void PianoRoll::horizontalZoom(int mag, const QPoint& glob_pos)
{
  QPoint cp = canvas->mapFromGlobal(glob_pos);
  QPoint sp = splitter->mapFromGlobal(glob_pos);
  if(cp.x() >= 0 && cp.x() < canvas->width() && sp.y() >= 0 && sp.y() < splitter->height())
    hscroll->setMag(hscroll->mag() + mag, cp.x());
}

//---------------------------------------------------------
//   updateHScrollRange
//---------------------------------------------------------

void PianoRoll::updateHScrollRange()
{
      int s, e;
      canvas->range(&s, &e);
      // Show one more measure.
      e += MusEGlobal::sigmap.ticksMeasure(e);  
      // Show another quarter measure due to imprecise drawing at canvas end point.
      e += MusEGlobal::sigmap.ticksMeasure(e) / 4;
      // Compensate for the fixed piano and vscroll widths. 
      e += canvas->rmapxDev(_pianoWidth - vscroll->width());
      int s1, e1;
      hscroll->range(&s1, &e1);
      if(s != s1 || e != e1) 
        hscroll->setRange(s, e);
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
//   cmd
//    pulldown menu commands
//---------------------------------------------------------

void PianoRoll::cmd(int cmd)
      {
        // Don't process if user is dragging or has clicked on the events. 
        // Causes crashes later in Canvas::viewMouseMoveEvent and viewMouseReleaseEvent.
        if(canvas->getCurrentDrag())
          return;
        
      MusECore::TagEventList tag_list;

      const FunctionDialogElements_t fn_element_dflt =
        FunctionAllEventsButton |
        FunctionSelectedEventsButton |
        FunctionLoopedButton |
        FunctionSelectedLoopedButton |
        FunctionAllPartsButton |
        FunctionSelectedPartsButton;
      
      switch (cmd)
            {
            case PianoCanvas::CMD_CUT:
                  tagItems(&tag_list, MusECore::EventTagOptionsStruct(MusECore::TagSelected | MusECore::TagAllParts));
                  MusECore::cut_items(&tag_list);
                  break;
            case PianoCanvas::CMD_COPY:
                  tagItems(&tag_list, MusECore::EventTagOptionsStruct(MusECore::TagSelected | MusECore::TagAllParts));
                  MusECore::copy_items(&tag_list);
                  break;
            case PianoCanvas::CMD_COPY_RANGE:
                  tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                    !itemsAreSelected(), true, true, MusEGlobal::song->lPos(), MusEGlobal::song->rPos()));
                  MusECore::copy_items(&tag_list);
                  break;
            case PianoCanvas::CMD_PASTE: 
                              ((PianoCanvas*)canvas)->cmd(PianoCanvas::CMD_SELECT_NONE);
                              MusECore::paste_items(partlist_to_set(parts()), 3072, MusECore::FunctionOptionsStruct(
                                  MusECore::FunctionEraseItemsDefault | MusECore::FunctionPasteNeverNewPart));
                              break;
            case PianoCanvas::CMD_PASTE_TO_CUR_PART: 
                              ((PianoCanvas*)canvas)->cmd(PianoCanvas::CMD_SELECT_NONE);
                              MusECore::paste_items(partlist_to_set(parts()), 3072, MusECore::FunctionOptionsStruct(
                                  MusECore::FunctionEraseItemsDefault | MusECore::FunctionPasteNeverNewPart),
                                  canvas->part());
                              break;
            case PianoCanvas::CMD_PASTE_DIALOG: 
                              ((PianoCanvas*)canvas)->cmd(PianoCanvas::CMD_SELECT_NONE);
                              MusECore::paste_items(partlist_to_set(parts()), (canvas->part()));
                              break;
                              
            case PianoCanvas::CMD_MODIFY_GATE_TIME:
                  {
                  FunctionDialogReturnGateTime ret =
                    gatetime_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::modify_notelen_items(&tag_list, ret._rateVal, ret._offsetVal);
                  }
                  break;
                  }
            case PianoCanvas::CMD_MODIFY_VELOCITY:
                  {
                  FunctionDialogReturnVelocity ret =
                    velocity_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::modify_velocity_items(&tag_list, ret._rateVal, ret._offsetVal);
                  }
                  break;
                  }
            case PianoCanvas::CMD_CRESCENDO:
                  {
                  FunctionDialogReturnCrescendo ret =
                    crescendo_items_dialog(FunctionDialogMode(
                      FunctionLoopedButton |
                      FunctionSelectedLoopedButton |
                      FunctionAllPartsButton | 
                      FunctionSelectedPartsButton));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::crescendo_items(&tag_list, ret._start_val, ret._end_val, ret._absolute);
                  }
                  break;
                  }
            case PianoCanvas::CMD_QUANTIZE:
                  {
                  FunctionDialogReturnQuantize ret =
                    quantize_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::quantize_items(&tag_list, ret._raster_index,
                                           ret._quant_len,
                                           ret._strength,
                                           ret._swing,
                                           ret._threshold);
                  }
                  break;
                  }
            
            case PianoCanvas::CMD_TRANSPOSE:
                  {
                  FunctionDialogReturnTranspose ret =
                    transpose_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::transpose_items(&tag_list, ret._amount);
                  }
                  break;
                  }
            case PianoCanvas::CMD_ERASE_EVENT:
            {
              FunctionDialogReturnErase ret =
                erase_items_dialog(FunctionDialogMode(fn_element_dflt));
              if(ret._valid)
              {
                tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                MusECore::erase_items(&tag_list, ret._veloThreshold, ret._veloThresUsed, ret._lenThreshold, ret._lenThresUsed);
              }
            }
            break;
            case PianoCanvas::CMD_DEL:
              tagItems(&tag_list, MusECore::EventTagOptionsStruct(MusECore::TagSelected | MusECore::TagAllParts));
              MusECore::erase_items(&tag_list);
              break;
            case PianoCanvas::CMD_NOTE_SHIFT:
                  {
                  FunctionDialogReturnMove ret =
                    move_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::move_items(&tag_list, ret._amount);
                  }
                  break;
                  }
            case PianoCanvas::CMD_FIXED_LEN:
                  {
                  FunctionDialogReturnSetLen ret =
                    setlen_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::set_notelen_items(&tag_list, ret._len);
                  }
                  break;
                  }
            case PianoCanvas::CMD_DELETE_OVERLAPS:
                  {
                  FunctionDialogReturnDelOverlaps ret =
                    deloverlaps_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::delete_overlaps_items(&tag_list);
                  }
                  break;
                  }
            case PianoCanvas::CMD_LEGATO:
                  {
                  FunctionDialogReturnLegato ret =
                    legato_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::legato_items(&tag_list, ret._min_len, !ret._allow_shortening);
                  }
                  break;
                  }
            
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
        if(veloOnValue == 0)
        {
          veloOnValue = 1;
          fprintf(stderr, "PianoRoll::setSelection: Warning: Zero note on velocity!\n");
        }
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
            info->setValues(0, 0, 0, deltaMode ? 0 : 1, 0);
            firstValueSet = false;
            tickValue     = 0;
            lenValue      = 0;
            pitchValue    = 0;
            veloOnValue   = 1;
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
//   ctrlPopupTriggered
//---------------------------------------------------------

void PianoRoll::ctrlPopupTriggered(QAction* act)
{
  // TODO Merge most of this with duplicate code in drum edit,
  //       maybe by putting it in a new function near populateMidiCtrlMenu. 
  
  if(!act || (act->data().toInt() == -1))
    return;
  
  int newCtlNum = -1;
  MusECore::Part* part       = curCanvasPart();
  MusECore::MidiTrack* track = (MusECore::MidiTrack*)(part->track());
  int channel      = track->outChannel();
  MusECore::MidiPort* port   = &MusEGlobal::midiPorts[track->outPort()];
  MusECore::MidiCtrlValListList* cll = port->controller();
  const int min = channel << 24;
  const int max = min + 0x1000000;
  const int edit_ins = max + 3;
  const int velo = max + 0x101;
  int rv = act->data().toInt();
  
  if (rv == velo) {    // special case velocity
        newCtlNum = MusECore::CTRL_VELOCITY;
        }
  else if (rv == edit_ins) {             // edit instrument
        MusECore::MidiInstrument* instr = port->instrument();
        MusEGlobal::muse->startEditInstrument(instr ? instr->iname() : QString(), EditInstrumentControllers);
        }
  else {                           // Select a control
        if(cll->find(channel, rv) == cll->end())
          cll->add(channel, new MusECore::MidiCtrlValList(rv));
        newCtlNum = rv;
        if(port->drumController(rv))
          newCtlNum |= 0xff;
      }

  if(newCtlNum != -1)
  {
    CtrlEdit* ctrlEdit = new CtrlEdit(splitter, this, _viewState.xscale(), _canvasXOrigin, 0, false, "pianoCtrlEdit");  
    ctrlEdit->setController(newCtlNum);
    setupNewCtrl(ctrlEdit);
  }
}

//---------------------------------------------------------
//   addCtrlClicked
//---------------------------------------------------------

void PianoRoll::addCtrlClicked()
{
  PopupMenu* pup = new PopupMenu(true);  // true = enable stay open. Don't bother with parent. 
  connect(pup, &QMenu::triggered, [this](QAction* act) { ctrlPopupTriggered(act); } );
  
  /*int est_width =*/ populateMidiCtrlMenu(pup, parts(), curCanvasPart(), curDrumInstrument());
  
  QPoint ep = addctrl->mapToGlobal(QPoint(0,0));
  //int newx = ep.x() - ctrlMainPop->width();  // Too much! Width says 640. Maybe because it hasn't been shown yet  .
//   int newx = ep.x() - est_width;  
//   if(newx < 0)
//     newx = 0;
//   ep.setX(newx);
  pup->exec(ep);
  delete pup;
  
  addctrl->setDown(false);
}

//---------------------------------------------------------
//   ctrlMenuAboutToShow
//---------------------------------------------------------

void PianoRoll::ctrlMenuAboutToShow()
{
  // Clear the menu and delete the contents.
  // "Removes all the menu's actions. Actions owned by the menu and not shown
  //  in any other widget are deleted."
  addControllerMenu->clear();
  populateMidiCtrlMenu(addControllerMenu, parts(), curCanvasPart(), curDrumInstrument());
}

//---------------------------------------------------------
//   ctrlMenuAboutToHide
//---------------------------------------------------------

void PianoRoll::ctrlMenuAboutToHide()
{
  // Clear the menu and delete the contents, since it's going to be cleared
  //  and refilled anyway next time opened, so we can save memory.
  // "Removes all the menu's actions. Actions owned by the menu and not shown
  //  in any other widget are deleted."
// FIXME: This crashes, of course...
//   addControllerMenu->clear();
}

//---------------------------------------------------------
//   addCtrl
//---------------------------------------------------------

CtrlEdit* PianoRoll::addCtrl(int ctl_num)
      {
      CtrlEdit* ctrlEdit = new CtrlEdit(splitter, this, _viewState.xscale(), _canvasXOrigin, 0, false, "pianoCtrlEdit");
      ctrlEdit->setController(ctl_num);
      ctrlEdit->setPanelWidth(_pianoWidth);
      setupNewCtrl(ctrlEdit);
      return ctrlEdit;
      }

//---------------------------------------------------------
//   setupNewCtrl
//---------------------------------------------------------

void PianoRoll::setupNewCtrl(CtrlEdit* ctrlEdit)
{      
  connect(tools2,   SIGNAL(toolChanged(int)),   ctrlEdit, SLOT(setTool(int)));
  connect(hscroll,  SIGNAL(scrollChanged(int)), ctrlEdit, SLOT(setXPos(int)));
  connect(hscroll,  SIGNAL(scaleChanged(int)),  ctrlEdit, SLOT(setXMag(int)));
  connect(ctrlEdit, SIGNAL(timeChanged(unsigned)),   SLOT(setTime(unsigned)));
  connect(ctrlEdit, SIGNAL(destroyedCtrl(CtrlEdit*)), SLOT(removeCtrl(CtrlEdit*)));
  connect(ctrlEdit, SIGNAL(yposChanged(int)), toolbar, SLOT(setInt(int)));
  connect(ctrlEdit, SIGNAL(redirectWheelEvent(QWheelEvent*)), canvas, SLOT(redirectedWheelEvent(QWheelEvent*)));
  connect(piano,    SIGNAL(curSelectedPitchChanged(int)), SLOT(setCurDrumInstrument(int)));
  //connect(piano,    SIGNAL(curSelectedPitchChanged(int)), canvas, SLOT(setCurDrumInstrument(int)));
  connect(canvas,   SIGNAL(curPartHasChanged(MusECore::Part*)), ctrlEdit, SLOT(curPartHasChanged(MusECore::Part*)));

  setCurDrumInstrument(piano->curSelectedPitch());
      
  ctrlEdit->setTool(tools2->curTool());
  ctrlEdit->setXPos(hscroll->pos());
  ctrlEdit->setXMag(hscroll->getScaleValue());
  ctrlEdit->setPanelWidth(_pianoWidth);

  ctrlEdit->show();
  ctrlEditList.push_back(ctrlEdit);
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
//   getViewState
//---------------------------------------------------------

MusECore::MidiPartViewState PianoRoll::getViewState() const
{
  MusECore::MidiPartViewState vs;
  vs.setXScroll(hscroll->offset());
  vs.setYScroll(vscroll->offset());
  vs.setXScale(hscroll->getScaleValue());
  vs.setYScale(vscroll->getScaleValue());
  CtrlEdit* ce;
  for(CtrlEditList::const_iterator i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
    ce = *i;
    vs.addController(MusECore::MidiCtrlViewState(ce->ctrlNum(), ce->perNoteVel()));
    }
  return vs;
}

void PianoRoll::storeInitialViewState() const
{
  const MusECore::PartList* pl = parts();
  if(pl)
  {
    const MusECore::MidiPartViewState vs = getViewState();
    for(MusECore::ciPart i = pl->begin(); i != pl->end(); ++i)
      i->second->setViewState(vs);
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

      storeSettings();
    
      emit isDeleting(static_cast<TopWin*>(this));
      e->accept();
      }

void PianoRoll::storeSettings() {

    QSettings settings;
    settings.setValue("Pianoroll/windowState", saveState());

    //Store values of the horizontal splitter
    QList<int> sizes = hsplitter->sizes();
    QList<int>::iterator it = sizes.begin();
    _trackInfoWidthInit = *it; //There are only 2 values stored in the sizelist, size of trackinfo widget and canvas widget
    it++;
    _canvasWidthInit = *it;
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
                        else if (tag == "trackinfowidth")
                              _trackInfoWidthInit = xml.parseInt();
                        else if (tag == "canvaswidth")
                              _canvasWidthInit = xml.parseInt();
                        else if (tag == "colormode")
                              colorModeInit = (MidiEventColorMode) xml.parseInt();
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
      xml.intTag(level, "trackinfowidth", _trackInfoWidthInit);
      xml.intTag(level, "canvaswidth", _canvasWidthInit);
      xml.intTag(level, "colormode", (int)colorModeInit);
      TopWin::writeConfiguration(PIANO_ROLL, level, xml);
      xml.etag(level, "pianoroll");
      }

//---------------------------------------------------------
//   soloChanged
//    signal from solo button
//---------------------------------------------------------

void PianoRoll::soloChanged(bool flag)
      {
      if(canvas->track())
      {
        // This is a minor operation easily manually undoable. Let's not clog the undo list with it.
        MusECore::PendingOperationList operations;
        operations.add(MusECore::PendingOperationItem(canvas->track(), flag, MusECore::PendingOperationItem::SetTrackSolo));
        MusEGlobal::audio->msgExecutePendingOperations(operations, true);
      }
      }

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void PianoRoll::setRaster(int val)
      {
      // Request to set the raster, but be sure to use the one it chooses,
      //  which may be different than the one requested.
      val = _rasterizerModel->checkRaster(val);
      MidiEditor::setRaster(val);
      _rasterInit = _raster;
      time->setRaster(_raster);
      canvas->redrawGrid();
      for (auto it : ctrlEditList)
          it->redrawCanvas();
      focusCanvas();     // give back focus after kb input
      }

//---------------------------------------------------------
//   changeRaster
//---------------------------------------------------------

int PianoRoll::changeRaster(int val)
      {
      // Request to set the raster, but be sure to use the one it chooses,
      //  which may be different than the one requested.
      MidiEditor::setRaster(toolbar->changeRaster(val));
      time->setRaster(_raster);
      canvas->redrawGrid();
      for (auto it : ctrlEditList)
          it->redrawCanvas();
      return _raster;
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
      if(hsplitter)
        hsplitter->writeStatus(level, xml);  
      
      for (std::list<CtrlEdit*>::const_iterator i = ctrlEditList.begin();
         i != ctrlEditList.end(); ++i) {
            (*i)->writeStatus(level, xml);
            }

      xml.intTag(level, "steprec", canvas->steprec());
      xml.intTag(level, "midiin", canvas->midiin());
      xml.intTag(level, "tool", int(canvas->tool()));
      xml.intTag(level, "playEvents", _playEvents);
      xml.intTag(level, "playEventsMode", _playEventsMode);
      xml.intTag(level, "xmag", hscroll->mag());
      xml.intTag(level, "xpos", hscroll->pos());
      xml.intTag(level, "ymag", vscroll->mag());
      xml.intTag(level, "ypos", vscroll->pos());
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
                        else if (hsplitter && tag == hsplitter->objectName())
                              hsplitter->readStatus(xml);
                        else if (tag == "playEvents") {
                              _playEvents = xml.parseInt();
                              canvas->setPlayEvents(_playEvents);
                              speaker->setChecked(_playEvents);
                              }
                        else if (tag == "playEventsMode") {
                              setSpeakerMode(EventCanvas::PlayEventsMode(xml.parseInt()));
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
                              changeRaster(_raster);
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   viewKeyPressEvent
//---------------------------------------------------------
void PianoRoll::keyPressEvent(QKeyEvent* event)
      {
      if (info->hasFocus()) {
            event->ignore();
            return;
            }

      RasterizerModel::RasterPick rast_pick = RasterizerModel::NoPick;
      const int cur_rast = raster();

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
      else if (key == shortcuts[SHRT_TOOL_PAN].key) {
            tools2->set(MusEGui::PanTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_ZOOM].key) {
            tools2->set(MusEGui::ZoomTool);
            return;
            }
      else if (key == shortcuts[SHRT_INSTRUMENT_STEP_UP].key) {
            piano->setCurSelectedPitch(piano->curSelectedPitch()+1);
            MusEGlobal::song->update(SC_DRUMMAP);
            return;
            }
      else if (key == shortcuts[SHRT_INSTRUMENT_STEP_DOWN].key) {
            piano->setCurSelectedPitch(piano->curSelectedPitch()-1);
            MusEGlobal::song->update(SC_DRUMMAP);
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
      else if (key == shortcuts[SHRT_EVENT_COLOR].key) {
            colorMode = MidiEventColorMode(int(colorMode) + 1);
            if (colorMode == MidiEventColorMode::lastInList)
                    colorMode = MidiEventColorMode::blueEvents;
            setEventColorMode(colorMode);
            return;
            }
      else if (key == shortcuts[SHRT_MOVE_PLAY_TO_NOTE].key){
        movePlayPointerToSelectedEvent();
        return;
      }
      else if (key == shortcuts[SHRT_STEP_RECORD].key) {
          canvas->setSteprec(!srec->isChecked());
          srec->setChecked(!srec->isChecked());
          return;
      }
      else if (key == shortcuts[SHRT_MIDI_INPUT].key) {
          canvas->setMidiin(!midiin->isChecked());
          midiin->setChecked(!midiin->isChecked());
          return;

      }
      else if (key == shortcuts[SHRT_PLAY_EVENTS].key) {
          canvas->setPlayEvents(!speaker->isChecked());
          speaker->setChecked(!speaker->isChecked());
          return;
      }
      else if (key == shortcuts[SHRT_INC_VELOCITY].key) {
          MusECore::TagEventList tag_list;
          tagItems(&tag_list, MusECore::EventTagOptionsStruct(MusECore::TagSelected | MusECore::TagAllParts));
          MusECore::modify_velocity_items(&tag_list, 100, 1);
          return;
      }
      else if (key == shortcuts[SHRT_DEC_VELOCITY].key) {
          MusECore::TagEventList tag_list;
          tagItems(&tag_list, MusECore::EventTagOptionsStruct(MusECore::TagSelected | MusECore::TagAllParts));
          MusECore::modify_velocity_items(&tag_list, 100, -1);
          return;
      }
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
          toolbar->setRaster(_raster);
        }
      }

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

void PianoRoll::eventColorModeChanged(MidiEventColorMode mode)
      {
      colorMode = mode;
      colorModeInit = colorMode;
      
      ((PianoCanvas*)(canvas))->setColorMode(colorMode);
      }

//---------------------------------------------------------
//   setEventColorMode
//---------------------------------------------------------

void PianoRoll::setEventColorMode(MidiEventColorMode mode)
      {
      colorMode = mode;
      colorModeInit = colorMode;
      
      evColorBlueAction->setChecked(mode == MidiEventColorMode::blueEvents);
      evColorPitchAction->setChecked(mode == MidiEventColorMode::pitchColorEvents);
      evColorVelAction->setChecked(mode == MidiEventColorMode::velocityColorEvents);
      
      ((PianoCanvas*)(canvas))->setColorMode(colorMode);
      }

//---------------------------------------------------------
//   clipboardChanged
//---------------------------------------------------------

void PianoRoll::clipboardChanged()
      {
      const bool has_gel = QApplication::clipboard()->mimeData()->hasFormat(QString("text/x-muse-groupedeventlists"));
      editPasteAction->setEnabled(has_gel);
      editPasteToCurPartAction->setEnabled(has_gel);
      editPasteDialogAction->setEnabled(has_gel);
      }

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void PianoRoll::selectionChanged()
      {
      bool flag = itemsAreSelected();
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
      canvas->setPlayEvents(_playEvents);
      }

void PianoRoll::setSpeakerSingleNoteMode(bool)
{
  setSpeakerMode(EventCanvas::PlayEventsSingleNote);
}

void PianoRoll::setSpeakerChordMode(bool)
{
  setSpeakerMode(EventCanvas::PlayEventsChords);
}

void PianoRoll::setSpeakerMode(EventCanvas::PlayEventsMode mode)
{
  _playEventsMode = mode;
  canvas->setPlayEventsMode(_playEventsMode);
  switch(_playEventsMode)
  {
    case EventCanvas::PlayEventsSingleNote:
      speaker->setIcon(*speakerSingleNoteSVGIcon);
    break;

    case EventCanvas::PlayEventsChords:
      speaker->setIcon(*speakerChordsSVGIcon);
    break;
  }
}

//---------------------------------------------------------
//   setupHZoomRange
//---------------------------------------------------------

void PianoRoll::setupHZoomRange()
{
  const int min = (_minXMag * MusEGlobal::config.division) / 384;
  hscroll->setScaleRange(min, _maxXMag);
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
      editPasteToCurPartAction->setShortcut(shortcuts[SHRT_PASTE_TO_CUR_PART].key);
      editPasteDialogAction->setShortcut(shortcuts[SHRT_PASTE_DIALOG].key);
      editDelEventsAction->setShortcut(shortcuts[SHRT_DELETE].key);
      
      selectAllAction->setShortcut(shortcuts[SHRT_SELECT_ALL].key); 
      selectNoneAction->setShortcut(shortcuts[SHRT_SELECT_NONE].key);
      selectInvertAction->setShortcut(shortcuts[SHRT_SELECT_INVERT].key);
      selectInsideLoopAction->setShortcut(shortcuts[SHRT_SELECT_ILOOP].key);
      selectOutsideLoopAction->setShortcut(shortcuts[SHRT_SELECT_OLOOP].key);
      selectPrevPartAction->setShortcut(shortcuts[SHRT_SELECT_PREV_PART].key);
      selectNextPartAction->setShortcut(shortcuts[SHRT_SELECT_NEXT_PART].key);
      startListEditAction->setShortcut(shortcuts[SHRT_OPEN_LIST].key);

//      eventColor->menuAction()->setShortcut(shortcuts[SHRT_EVENT_COLOR].key);
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
      MusEGlobal::song->executeScript(this, scriptfile.toLatin1().data(), parts(), raster(), true);
}

//---------------------------------------------------------
//   execUserScript
//---------------------------------------------------------
void PianoRoll::execUserScript(int id)
{
      QString scriptfile = MusEGlobal::song->getScriptPath(id, false);
      MusEGlobal::song->executeScript(this, scriptfile.toLatin1().data(), parts(), raster(), true);
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

} // namespace MusEGui
